import json
import datetime
from pymongo import MongoClient
from bson.objectid import ObjectId
import logging

# 로깅 설정
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("mongodb_transfer.log"),
        logging.StreamHandler()
    ]
)

CONFIG = {
	"FROM": {
		"host": "47.56.150.14",
		"user": "gas_user",
		"password": "13579",
		"db": "gas_common",
		"port": 5090
	},
	"TO": {
		"host": "140.245.72.44",
		"user": "gas_user",
		"password": "13579",
		"db": "gas_common",
		"port": 5090
	}
}

def connect_to_mongodb(host = '', user = '', password = '', db = '', charset ='', port=0):
	uri ="mongodb://%s:%s@%s:%d/" %(user, password, host, int(port)) 
	# Connect to MongoDB (default localhost:27017)
	client = MongoClient(uri, authSource='admin')
	return client

def getDatabaseTree(client):
    db_list = client.list_database_names()

    # 시스템 데이터베이스 제외 (선택사항)
    excluded_dbs = ['admin', 'config', 'local']
    db_list = [db for db in db_list if db not in excluded_dbs]

    arr = []  
    for db in db_list:
        dbs = client[db]
        
        # 해당 데이터베이스의 모든 컬렉션 목록 가져오기
        collection_list = dbs.list_collection_names()
        
        # 시스템 컬렉션 제외 (선택사항)
        excluded_collections = ['system.indexes', 'system.users']
        collection_list = [col for col in collection_list if col not in excluded_collections]
        arr.append({"db": db, "collections": collection_list})

    return arr

def get_unique_keys(collection):
    """컬렉션의 인덱스 정보를 확인하여 유니크 키를 찾습니다."""
    unique_keys = []
    indexes = collection.index_information()
    
    for index_name, index_info in indexes.items():
        if index_info.get('unique', False):
            # 유니크 인덱스의 키 필드들을 추출
            fields = [key[0] for key in index_info['key']]
            unique_keys.append(fields)
    
    return unique_keys

def document_exists(collection, document, unique_keys):
    """문서가 이미 컬렉션에 존재하는지 확인합니다."""
    if not unique_keys:
        # _id로 검색 (기본 유니크 키)
        if '_id' in document:
            return collection.count_documents({'_id': document['_id']}) > 0
        return False
    
    # 각 유니크 인덱스에 대해 검사
    for keys in unique_keys:
        query = {}
        has_all_keys = True
        
        for key in keys:
            if key in document:
                query[key] = document[key]
            else:
                has_all_keys = False
                break
        
        if has_all_keys and query and collection.count_documents(query) > 0:
            return True
    
    return False

def transfer_db():
    start_time = datetime.datetime.now()
    logging.info(f"데이터 전송 시작: {start_time}")
    
    from_client = connect_to_mongodb(
        host = CONFIG['FROM']['host'], 
        user = CONFIG['FROM']['user'], 
        password = CONFIG['FROM']['password'], 
        db = CONFIG['FROM']['db'], 
        port = CONFIG['FROM']['port']
    )

    to_client = connect_to_mongodb(
        host = CONFIG['TO']['host'], 
        user = CONFIG['TO']['user'], 
        password = CONFIG['TO']['password'], 
        db = CONFIG['TO']['db'], 
        port = CONFIG['TO']['port']
    )

    # 소스 서버의 데이터베이스 목록 가져오기
    from_db = getDatabaseTree(from_client)
    
    # 대상 서버의 데이터베이스 목록 가져오기
    to_db = getDatabaseTree(to_client)
    
    # 대상 서버의 DB와 컬렉션 구조를 딕셔너리로 변환
    to_db_dict = {db_info['db']: db_info['collections'] for db_info in to_db}
    
    # 통계 정보
    stats = {
        'total_databases': len(from_db),
        'total_collections': 0,
        'total_documents': 0,
        'transferred_documents': 0,
        'skipped_documents': 0,
        'errors': 0
    }

    logging.info(f"소스 서버에서 {stats['total_databases']}개의 데이터베이스를 발견했습니다.")
    
    # 각 데이터베이스와 컬렉션에 대해 처리
    for db_info in from_db:
        db_name = db_info['db']
        collections = db_info['collections']
        stats['total_collections'] += len(collections)
        
        logging.info(f"데이터베이스 처리 중: {db_name} (컬렉션 {len(collections)}개)")
        
        # 소스 데이터베이스 연결
        from_database = from_client[db_name]
        
        # 대상 데이터베이스 연결 (없으면 자동 생성)
        to_database = to_client[db_name]
        
        # 대상 서버에 해당 데이터베이스가 있는지 확인
        db_exists = db_name in to_db_dict
        if db_exists:
            logging.info(f"대상 서버에 데이터베이스 {db_name}이(가) 이미 존재합니다.")
        else:
            logging.info(f"대상 서버에 데이터베이스 {db_name}을(를) 생성합니다.")
        
        # 각 컬렉션 처리
        for collection_name in collections:
            logging.info(f"컬렉션 처리 중: {db_name}.{collection_name}")
            
            # 소스 컬렉션
            from_collection = from_database[collection_name]
            
            # 대상 컬렉션 (없으면 자동 생성)
            to_collection = to_database[collection_name]
            
            # 컬렉션이 대상 서버에 이미 존재하는지 확인
            collection_exists = db_exists and collection_name in to_db_dict.get(db_name, [])
            if collection_exists:
                logging.info(f"대상 서버에 컬렉션 {collection_name}이(가) 이미 존재합니다.")
            else:
                logging.info(f"대상 서버에 컬렉션 {collection_name}을(를) 생성합니다.")
            
            # 컬렉션의 유니크 키 정보 가져오기
            unique_keys = get_unique_keys(to_collection) if collection_exists else []
            
            # 문서 수 확인
            total_docs = from_collection.count_documents({})
            stats['total_documents'] += total_docs
            
            logging.info(f"{collection_name} 컬렉션에서 {total_docs}개 문서 처리 예정")
            
            # 배치 크기 설정
            batch_size = 100
            processed = 0
            
            # 문서 전송
            for document in from_collection.find():
                try:
                    # ObjectId 객체를 문자열로 변환하여 로깅 가능하게 함
                    doc_id = str(document.get('_id', 'unknown'))
                    
                    # 중복 검사
                    if document_exists(to_collection, document, unique_keys):
                        logging.debug(f"중복 문서 건너뜀: {doc_id}")  # 너무 많은 로그를 방지하기 위해 debug 레벨로 변경
                        stats['skipped_documents'] += 1
                    else:
                        # 문서 삽입
                        to_collection.insert_one(document)
                        stats['transferred_documents'] += 1
                        
                    processed += 1
                    if processed % batch_size == 0:
                        logging.info(f"{collection_name}: {processed}/{total_docs} 문서 처리됨 ({processed/total_docs*100:.1f}%)")
                        
                except Exception as e:
                    logging.error(f"문서 전송 오류 (ID: {doc_id}): {str(e)}")
                    stats['errors'] += 1
    
    # 연결 종료
    from_client.close()
    to_client.close()
    
    end_time = datetime.datetime.now()
    duration = end_time - start_time
    
    # 최종 통계 출력
    logging.info("=" * 50)
    logging.info("데이터 전송 완료")
    logging.info(f"총 소요 시간: {duration}")
    logging.info(f"처리된 데이터베이스 수: {stats['total_databases']}")
    logging.info(f"처리된 컬렉션 수: {stats['total_collections']}")
    logging.info(f"총 문서 수: {stats['total_documents']}")
    logging.info(f"전송된 문서 수: {stats['transferred_documents']}")
    logging.info(f"건너뛴 문서 수 (중복): {stats['skipped_documents']}")
    logging.info(f"오류 발생 수: {stats['errors']}")
    logging.info("=" * 50)

if __name__ == "__main__":
    transfer_db()

