# mongodb centos 설치
https://www.infracody.com/2022/05/install-mongodb-on-centos-7.html 참조
https://www.mongodb.com/ko-kr/docs/manual/installation/
https://www.mongodb.com/ko-kr/docs/manual/administration/install-community/

## Community Edition 설치



# 계정 추가하기
mongo 커맨드를 이용해 mongo shell을 연다.
 
use cool_db

db.createUser({
    user: 'ian',
    pwd: 'secretPassword',
    roles: [{ role: 'readWrite', db:'cool_db'}]
})
use 커맨드로 대상 db를 선택하고,
db.createUser 메서드로 인증 사용자를 추가한다.
 
여기에서는 대상 db가 cool_db, 
사용자의 id가 ian,
사용자의 비밀번호가 secretPassword,
권한은 cool_db에 대해 읽기&쓰기 권한을 부여한다.
 
사용자가 성공적으로 생성되었다면 쉘을 빠져나온다.

#인증을 사용하도록 /etc/mongod.conf 수정
security:
  authorization: 'enabled'
맨 아래에 다음을 추가한다.
 
그리고 나서 다시 net: 쪽으로 돌아가서
.......
net:
  port: 27017
  #bindIp: 127.0.0.1
  bindIp: 0.0.0.0
.......
외부 접속을 모두 허용한다.
 
이제 인증이 걸려있기 때문에 시도를 할 수는 있지만 인증 정보가 없으면 접근이 불가능하다.
 
이것도 불안하다면
 
.......
net:
  port: 27017
  bindIp: 127.0.0.1,123.456.789.123
  #bindIp: 0.0.0.0
.......
처럼 127.0.0.1옆에 컴마를 찍은 후 접근 허용할 아이피를 입력한다.
 
컴마 옆에 공백문자를 넣지 않도록 주의한다.
 
sudo service mongod restart
몽고를 재시작한다.
 
인증이 잘 먹는지 로컬에서 실험해보자.
mongo
정상적으로 완료되었다면 이제 mongo 명령어로는 db에 접근할 수 없다.
 
mongo -u [USER_ID] -p [PASSWORD] 127.0.0.1/[DB_NAME]
이렇게 하면 그 DB에 접근할 수 있게 된다.
 
외부에서도 동일한 방법으로 실행할 수 있다.
 
mongo -u [USER_ID] -p [PASSWORD] [YOUR_MONGO_SERVER_IP]/[DB_NAME]
 
출처: https://satisfactoryplace.tistory.com/338 [만족:티스토리]

사용자 목록 출력
사용자 계정은 각 데이터베이스의 db.system.users 컬렉션에 저장된다.

> use admin
> show users

or

> db.getUsers();
 

사용자 계정 생성
db.createUser({ 
    user: '이름',
    pwd: '비밀번호', 
    roles: ['root'] // 계정이 가질 권한
});
// read 권한만 갖고 있는 동일한 사용자를 admin 데이터베이스에 추가하고 
// testDB2 데이터베이스에 대한 readWrite권한 부여
db.createUser( { 
    user: "testUser" ,
    userSource: "test",
    roles: ["read"], 
    otherDBRoles : { testDB2: ["readWrite"] } 
} )
 

사용자 확인 (로그인)
리턴값으로 1이 출력되면 로그인 성공이다.

db.auth("아이디", "비밀번호")
사용자계정관리
 

사용자 삭제
db.dropUser("유저이름")
사용자계정관리

db.createUser({
  user: "gas_user",
  pwd: '13579',      
  roles: [
        { role: "readWrite", db: "gas_common" },
        { role: "readWrite", db: "gas_demo" },
  ],
})

db.grantRolesToUser(
    "gas_user",
    [
        { role: "readWrite", db: "gas_common" },
        { role: "readWrite", db: "gas_demo" },
    ]
)

db.grantRolesToUser(
    "gas_user",
    [
        { role: "readWrite", db: "/^gas_/" },
    ]
)
```
db.revokeRolesFromUser( "gas_user",
    [ { role: "readWrite", db: "gas_" }, "readWrite" ],
)
```

set unique
db.user_route.createIndex({ ID: 1 }, { unique: true })


## [MongoDB] 쉘 Command 모음
1. Create
(1) DB
> use [dbname]
[dbname]이라는 DB를 생성해주거나 [dbname]로 Switching 해준다

(2) Collection&Document
> db.collection.insert(
  [document or array of documents],
  {
    writeConcern: [document],
    ordered: [boolean] # true면 document array의 인덱스 순으로 insert. default는 True
  }
)

WriteResult({ "nInserted" : 1 })
document 하나를 insert하면 collection이 함께 생성된다.

2. Read
(1) DB
> show dbs
local		0.000GB
dotori		0.070GB

> db
dotori
worktime
(2) Collection
> show collections
items
users
adminsettings

> db.getCollectionNames()
["items", "users", "adminsettings"]
(3) 기타
용량 확인
> use [dbname]
> db.stats()
{
	"db" : "test",
	"collections" : 0,
	"views" : 0,
	"objects" : 0,
	"avgObjSize" : 0,
	"dataSize" : 0,
	"storageSize" : 0, # 실제 크기
	"numExtents" : 0,
	"indexes" : 0,
	"indexSize" : 0,
	"scaleFactor" : 1,
	"fileSize" : 0,
	"fsUsedSize" : 0,
	"fsTotalSize" : 0,
	"ok" : 1
}
(3) Document
find() 메소드
> db.[collection].find([query],[projection])
find() 메소드를 사용하여 collection 내의 document를 select 한다.

Parameter	Type	Description
query	Document	Option. Document를 Select 할 기준
projection	Document	Option. Document select 결과에 포함될 fields

collection 내 모든 document select
> db.items.find()
{ "_id" : ObjectId("5f963733a7540da231c3e759"), "author" : "test", "title" : "test", "tags" : [ "rttt", "maya", "texture", "scene" ]}

> db.items.find().pretty() # 정리해서 출력
{
	"_id" : ObjectId("5f963733a7540da231c3e759"),
	"author" : "test",
	"title" : "test",
	"tags" : [
		"rttt",
		"maya",
		"texture",
		"scene"
	]
}

query, field 작성 예시
> db.items.find(
... {title:"test"},
... {_id:0,title:1,tag:1}
... )

{ "title" : "test", "tags" : [ "rttt", "maya", "texture", "scene" ] }

비교 연산자
> db.books.find(
... {price:{$gt:100,$lte:400}},
... {_id:0,title:1,tag:1}
... )
Operator	Meaning
$eq	equals
$gt	greater than
$gte	greater than or equals
$lt	less than
$lte	less than or equals
$ne	not equal
$in	in an array
$nin	not in an array

논리 연산자
db.books.find(
  { $or: [ { price: { $lt: 200 } }, { stock: 0 } ] }
)

3. Update
> db.[collection].update(
  [query],
  [update],
  {
   upsert: [boolean], # true 일 때, query에 매칭되는 document가 없으면 새로 insert 
   multi: [boolean],  # false 일 때, 하나만 update
   writeConcern: [document]
  }
)
writeConcern

예시
db.books.update(
  { price: { $gt: 200 } },
  { $set: { author: "Kim" } },
  { multi: true }
)
price가 200보다 큰 document를 찾아, author를 kim으로 바꿔주는 명령
```
gas_common> db.device_route.update(
    { usn:'A5533783'},
    { $set: {db_name:'gas_demo'}}
)
```
Update field Operators
Operator	Description
$inc	field의 value를 지정한 수만큼 증가시킨다.
$rename	field 이름을 rename한다.
$setOnInsert	update()의 upsert가 true로 설정되었을 경우, document가 insert될 때의 field value를 설정한다.
$set	update할 field의 value를 설정한다.
$unset	document에서 설정된 field를 삭제한다
$min	설정값이 field value보다 작은 경우만 update한다.
$max	설정값이 field value보다 큰 경우만 update한다.
$currentDate	현재 시간을 설정한다

참고하면 좋을 사이트
4. Delete
(1) DB
> db.dropDatabase();
{ "dropped" : "dotori", "ok" : 1 }
(2) Collection
db.items.drop()
(3) Document
> db.collection.remove(
  [query],
  {
    justOne: [boolean],
    writeConcern: [document]
  }
)
(4) Field
> db.collection.update(
   [query],
    {
      $unset:{fieldname:1}
      }
)
Reference
https://poiemaweb.com/mongdb-basics-shell-crud
https://docs.mongodb.com/manual/crud/#read-operations