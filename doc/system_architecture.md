# 가스 미터 데이터 수집 시스템 아키텍처

## 1. 프로젝트 개요

이 프로젝트는 가스 미터 데이터를 수집하고 처리하기 위한 TCP 서버 시스템입니다. C 언어로 구현되었으며 MongoDB와 MySQL 두 가지 데이터베이스 시스템을 지원합니다.

## 2. 시스템 구조

### 2.1 코어 서버 모듈 (server.c)

- TCP 소켓 기반 서버 구현
- 멀티스레드로 클라이언트 연결 처리
- 클라이언트 요청 처리 및 응답 생성
- 명확한 에러 처리 로직

```c
int main(){
  // 서버 설정 로드 및 초기화
  load_config();
  server_fd = createServer();

  while (1) {
    // 클라이언트 연결 수락
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    
    // 스레드 생성하여 클라이언트 처리
    pthread_t thread_id;
    int* client_fd_ptr = malloc(sizeof(int));
    *client_fd_ptr = client_fd;
    pthread_create(&thread_id, NULL, thread_proc_message, client_fd_ptr);
    pthread_detach(thread_id);
  }
}
```

### 2.2 메시지 처리 모듈 (message.h/c)

- 프로토콜 구조체 정의
- 패킷 파싱 및 생성 함수
- 시간 및 날짜 처리 유틸리티

**주요 구조체**:
```c
// 수신 패킷 구조체
typedef struct {
  unsigned char eid;
  char uid[8];
  struct count counts[24];
  unsigned char bat;
  unsigned int uptime;
} recv_packet;

// 송신 패킷 구조체
typedef struct {
  unsigned char ret;
  char uid[8];
  unsigned int tsc;
  unsigned int tsn;
  unsigned int cnt;
  unsigned short min;
  unsigned short max;
  unsigned char svr[4];
  unsigned int prt;
} send_packet;
```

### 2.3 데이터베이스 모듈

#### MongoDB 모듈 (proc_mongo.h/c)

- libmongoc 라이브러리 사용
- 디바이스 정보 저장 및 검색
- 카운터 데이터 관리
- BSON 문서 생성 및 쿼리 처리

#### MySQL 모듈 (proc_mysql.h/c)

- libmysqlclient 라이브러리 사용
- SQL 쿼리 생성 및 실행
- 결과 처리 및 메모리 관리
- 에러 처리 매크로 사용

**공통 구조체**:
```c
// 디바이스 정보 구조체
typedef struct {
  unsigned int pk;
  char device_uid[128];
  char meter_id[128];
  // ... 기타 필드들
  unsigned int status;
  char flag;
} Table_Device;
```

### 2.4 설정 관리 모듈 (load_config.h/c)

- JSON 설정 파일 파싱
- parson 라이브러리 사용
- 데이터베이스, 서버, 타임존 등 설정 관리

### 2.5 JSON 파싱 라이브러리 (parson.h/c)

- 경량 JSON 파싱 라이브러리
- JSON 객체, 배열, 값 처리
- 메모리 관리 및 직렬화/역직렬화

### 2.6 빌드 시스템 (Makefile)

- GCC를 사용한 컴파일 설정
- MongoDB/MySQL 지원 설정
- pkg-config를 통한 라이브러리 플래그 관리
- 프로덕션 및 개발 빌드 지원

## 3. 프로토콜 구조

### 3.1 업스트림 프로토콜 (디바이스 → 서버)

총 256바이트 패킷:
```
[이벤트ID(1B)][UID(8B)][카운트 테이블(192B)][배터리(1B)][가동 시간(4B)][이벤트 코드(1B)][회전 시간(2B)][예약(47B)]
```

- **이벤트 ID**: 패킷 유형 (0xA0: 정상, 0xE0: 이벤트, 0xE1: 초기화)
- **UID**: 디바이스 고유 식별자
- **카운트 테이블**: 24개 시간별 카운터 값 (timestamp + count)
- **배터리**: 배터리 잔량 (0-100%)
- **가동 시간**: 디바이스 동작 시간 (초)

### 3.2 다운스트림 프로토콜 (서버 → 디바이스)

총 32바이트 패킷:
```
[RC(1B)][UID(8B)][TS(4B)][NextTS(4B)][Count(4B)][Min(2B)][Max(2B)][HOST(4B)][PORT(2B)][FLAG(1B)]
```

- **RC (Return Code)**: 비트 플래그 형태의 응답 코드
  - 비트 7: TS 변경
  - 비트 6: Min/Max 변경
  - 비트 3: Count 값 변경
  - 비트 2: 서버 정보 변경
  - 비트 1: UID 변경
  - 비트 0: 패리티 비트
- **NextTS**: 다음 통신 시간 (0: 즉시, 0xFFFFFFFF: 통신 중지)

## 4. 데이터 흐름

1. **디바이스 연결 수립**:
   - 디바이스가 TCP 5004 포트로 서버에 연결
   - 서버는 별도 스레드에서 연결 처리

2. **패킷 처리**:
   - 디바이스로부터 256바이트 패킷 수신
   - `parse_recv()` 함수로 패킷 파싱
   - 이벤트 ID에 따라 처리 로직 결정

3. **데이터베이스 처리**:
   - 디바이스 정보 조회 (`device_info()`)
   - 카운터 데이터 저장 (`update_counter()`)
   - 디바이스 상태 업데이트 (`update_device()`)

4. **응답 생성 및 전송**:
   - `mk_send_packet()` 함수로 32바이트 응답 패킷 생성
   - 클라이언트에 응답 전송
   - 연결 종료

## 5. 주요 기능

### 5.1 디바이스 관리

- 디바이스 등록 및 인증
- 배터리 및 가동 시간 모니터링
- 디바이스 설정 관리 (최소/최대 값, 서버 정보)

### 5.2 데이터 수집

- 시간별 카운터 값 수집 및 저장
- 일/주/월별 데이터 집계를 위한 타임스탬프 처리
- 정상 상태 및 이벤트 데이터 구분

### 5.3 서버 관리

- 설정 파일 기반 서버 구성
- 다중 데이터베이스 지원
- 멀티스레드 처리로 확장성 확보

## 6. 시스템 배포

서버는 Linux systemd 서비스로 운영됩니다:

```
[Unit]
Description=Staring gasDataServer Service by Hans
After=multi-user.target

[Service]
WorkingDirectory=/home/hanskim/gasDataServer
ExecStart=/home/hanskim/gasDataServer/server1
Restart=always
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

- 자동 재시작 기능 활성화
- 다중 사용자 환경 이후 시작

## 7. 코드 품질 분석

### 7.1 장점

1. **모듈화된 설계**:
   - 기능별 파일 분리로 유지보수성 향상
   - 명확한 인터페이스를 통한 모듈 간 통신

2. **유연한 데이터베이스 지원**:
   - MongoDB와 MySQL 모두 지원
   - 동일한 인터페이스로 다른 DB 백엔드 사용 가능

3. **효율적인 메모리 관리**:
   - 특히 proc_mongo.c에서 BSON 객체 해제 관리
   - 버퍼 오버플로우 방지를 위한 안전한 메모리 접근

4. **세밀한 오류 처리**:
   - 함수별 오류 상태 반환
   - 에러 로깅 및 리소스 정리

### 7.2 개선 사항

1. **코드 중복**:
   - proc_mongo.c와 proc_mysql.c에 유사한 기능의 함수들
   - 데이터베이스 추상화 계층 필요

2. **보안 문제**:
   - 평문 비밀번호 사용
   - 통신 암호화 부재

3. **일관성 부족**:
   - 코딩 스타일 혼합
   - 에러 처리 방식의 차이

4. **문서화 부족**:
   - 코드 주석 부족
   - 프로토콜 명세 불완전

## 8. 개선 제안

1. **코드 구조 개선**:
   - 데이터베이스 접근 계층 추상화
   - 공통 유틸리티 함수 통합

2. **보안 강화**:
   - 비밀번호 암호화 또는 환경 변수 사용
   - 통신 암호화(TLS/SSL) 구현

3. **에러 처리 강화**:
   - 일관된 에러 코드 및 로깅 시스템
   - 메모리 누수 가능성 점검

4. **테스트 코드 추가**:
   - 단위 테스트 구현
   - 시나리오 기반 통합 테스트

5. **문서화 개선**:
   - 코드 주석 추가
   - README 파일 작성 및 프로토콜 명세화

## 9. 결론

이 프로젝트는 가스 미터 데이터를 수집하고 관리하기 위한 잘 설계된 서버 시스템입니다. C 언어로 작성되었으며 MongoDB와 MySQL을 모두 지원합니다. 코드는 모듈화되어 있고 확장 가능하도록 설계되었으나, 일부 중복 코드와 보안 문제가 있습니다.

효율적인 프로토콜 설계와 멀티스레드 처리는 시스템의 강점이지만, 데이터베이스 접근 추상화와 보안 강화가 필요합니다. 전체적으로, 이 코드베이스는 IoT 데이터 수집 시스템의 견고한 기반을 제공합니다. 