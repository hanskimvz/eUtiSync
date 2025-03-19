# eUtiSync 프로젝트

## 프로젝트 개요

eUtiSync는 가스 미터 데이터를 수집하고 처리하기 위한 통합 시스템입니다. 이 프로젝트는 NB-IoT 네트워크를 통해 가스 미터 디바이스의 데이터를 수집하고, TCP 서버를 통해 데이터를 처리하며, MongoDB와 MySQL 데이터베이스를 활용하여 데이터를 저장하고 관리합니다.

## 시스템 구조

### 주요 컴포넌트

1. **TCP 서버 (server.c)**
   - 디바이스 연결 수립 및 관리
   - 멀티스레드 방식으로 다중 클라이언트 처리
   - 패킷 수신 및 파싱, 응답 생성

2. **데이터베이스 모듈**
   - MongoDB 인터페이스 (proc_mongo.c/h)
   - MySQL 인터페이스 (proc_mysql.c/h)
   - 디바이스 정보 및 카운터 데이터 저장

3. **메시지 처리 모듈 (message.c/h)**
   - 프로토콜 구조체 정의
   - 패킷 파싱 및 생성
   - 시간/날짜 처리 유틸리티

4. **설정 관리 모듈 (load_config.c/h)**
   - JSON 설정 파일 파싱
   - 서버 및 데이터베이스 설정 관리

5. **제조 모듈 (manufacture/)**
   - 디바이스 시리얼 번호 할당 (serializer.c)
   - 프로덕션 서버 (prod_server.c)

### 폴더 구조

```
eUtiSync/
├── doc/                   # 문서 디렉토리
│   ├── protocol.md        # 프로토콜 명세서
│   ├── system_architecture.md # 시스템 아키텍처 문서
│   └── ...                # 기타 문서 파일
├── manufacture/           # 제조 관련 모듈
│   ├── serializer.c       # 디바이스 시리얼 할당 프로그램
│   ├── prod_server.c      # 프로덕션 서버
│   └── Makefile           # 제조 모듈 빌드 스크립트
├── migration/             # 데이터 마이그레이션 스크립트
├── .vscode/               # VSCode 설정
├── server.c               # 메인 TCP 서버
├── proc_mongo.c           # MongoDB 처리 모듈
├── proc_mysql.c           # MySQL 처리 모듈
├── message.c              # 메시지 처리 모듈
├── load_config.c          # 설정 로드 모듈
├── parson.c               # JSON 파싱 라이브러리
├── Makefile               # 메인 빌드 스크립트
└── config.json            # 서버 설정 파일
```

## 프로토콜

### 업스트림 프로토콜 (디바이스 → 서버)

256바이트 패킷으로 구성:
- Event ID (1B): 패킷 유형 (0xA0, 0xB0, 0xE0, 0xE1)
- UID (8B): 디바이스 고유 식별자
- Count data (192B): 시간별 카운터 값
- Battery (1B): 배터리 잔량
- Uptime (4B): 디바이스 가동 시간
- Event code (1B): 이벤트 코드
- Time per rotation (2B): 회전당 시간
- Reserved (47B): 예약 공간

### 다운스트림 프로토콜 (서버 → 디바이스)

32바이트 패킷으로 구성:
- Return Code (1B): 변경 요소 플래그
- UID (8B): 디바이스 고유 식별자
- TS now (4B): 현재 타임스탬프
- TS next (4B): 다음 통신 타임스탬프
- Count value (4B): 카운터 값
- Min/Max speed (4B): 최소/최대 회전 속도
- Host/Port (6B): 서버 호스트 및 포트
- IO Flag (1B): IO 제어 플래그

자세한 내용은 [프로토콜 명세서](doc/protocol.md)를 참조하세요.

## 기능

1. **디바이스 데이터 수집**
   - 가스 미터 디바이스로부터 실시간 데이터 수집
   - 시간별 카운터 값, 배터리 상태, 가동 시간 등 정보 수집

2. **디바이스 관리**
   - 디바이스 등록 및 인증
   - 디바이스 설정 원격 변경 (통신 주기, 회전 속도 범위 등)
   - 디바이스 상태 모니터링

3. **데이터베이스 관리**
   - MongoDB와 MySQL 데이터베이스 지원
   - 디바이스 정보 및 카운터 데이터 저장
   - 데이터 마이그레이션 및 백업

4. **제조 프로세스 지원**
   - 새 디바이스 시리얼 번호 할당
   - 디바이스 초기 설정 및 프로비저닝

## 빌드 및 실행

### 요구 사항

- GCC 컴파일러
- MongoDB 및/또는 MySQL 클라이언트 라이브러리
- pkg-config
- POSIX 호환 시스템

### 빌드 방법

```bash
# 메인 서버 빌드
make

# 제조 모듈 빌드
cd manufacture
make
```

### 실행 방법

```bash
# 메인 서버 실행
./server

# 시리얼 할당 서버 실행 (제조 모듈)
cd manufacture
./serializer [새로운_USN]
```

## 설정

`config.json` 파일에서 다음 설정을 구성할 수 있습니다:

- 데이터베이스 연결 정보 (MongoDB/MySQL)
- 서버 포트 및 IP 주소
- 로깅 레벨 및 경로
- 타임존 설정

## 라이센스

이 프로젝트는 BSD 2-Clause 라이센스를 따릅니다. 자세한 내용은 소스 코드 상단의 라이센스 텍스트를 참조하세요.

## 문서

자세한 시스템 아키텍처 및 구현에 대한 정보는 다음 문서를 참조하세요:

- [시스템 아키텍처](doc/system_architecture.md)
- [프로토콜 명세서](doc/protocol.md)
- [MongoDB 정보](doc/mongodb.md)
- [NB-IoT TCP 프로토콜](doc/NB-IoT_TCP_Protocol.md)
