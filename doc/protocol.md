# 프로토콜 명세서

## 문서 정보
- **문서명**: 업스트림 및 다운스트림 프로토콜 명세서
- **작성자**: Hans Kim
- **날짜**: 2024-12-11
- **버전**: 1.0

## 1. 업스트림 프로토콜 (디바이스 → 서버)

### 1.1 패킷 구조

```
EventID(1Byte) | UID(8Byte) | Count table(192Byte) | Bat_Rem(1Byte) | Uptime(4Byte) | EVENT | ETC
```

### 1.2 패킷 상세 명세

- **총 길이**: 256 바이트

| 필드 | 위치 | 크기 | 설명 |
|------|------|------|------|
| Event ID | packet[0] | 1 바이트 | 패킷 유형 식별자 |
| UID | packet[1:9] | 8 바이트 | 디바이스 고유 식별자 (ASCII) |
| Count data | packet[9:201] | 192 바이트 | 카운트 데이터 테이블 |
| Battery Remaining | packet[201] | 1 바이트 | 배터리 잔량 (0~100%) |
| Uptime | packet[202:206] | 4 바이트 | 디바이스 가동 시간 (초) |
| Event code | packet[206] | 1 바이트 | 이벤트 코드 (Event ID가 0xE0일 때 유효) |
| Time per rotation | packet[207:209] | 2 바이트 | 회전당 소요 시간 (초) |
| Reserved | packet[209:256] | 47 바이트 | 향후 사용을 위한 예약 공간 (0xFF로 채움) |

### 1.3 필드 상세 설명

#### Event ID (1 바이트)
- **0xA0**: 정상 상태
- **0xB0**: 타임스탬프 (현재 사용 불가)
- **0xE0**: 이벤트 발생
- **0xE1**: 초기화 상태

#### UID (8 바이트)
- 8바이트 ASCII 문자로 구성된 디바이스 고유 식별자

#### Count data (192 바이트)
- 구조: `{Timestamp(4바이트) + Count(4바이트)} × 24`
- 타임스탬프는 EPOCH 형식으로 내림차순 정렬
- 초기화 상태: `{ts:0x0000 + count:0x0000} × 24`
- 정상 상태: `{ts: 과거 타임스탬프 + count: 누적 카운터 값} × 24`

#### Battery Remaining (1 바이트)
- 배터리 잔량 표시 (0x00~0x64, 0~100%)

#### Uptime (4 바이트)
- 디바이스 가동 시간 (초 단위)

#### Event code (1 바이트)
- Event ID가 0xE0일 때만 유효
- **0x01**: 외부 자기장 감지
- **0x02**: 회전 속도(간격) 위반

#### Time per rotation (2 바이트)
- 1회 회전당 소요 시간 (초 단위)
- 값 범위: 0~7200

## 2. 다운스트림 프로토콜 (서버 → 디바이스)

### 2.1 패킷 구조

```
RC(1B) | UID(8B) | TS(4B) | NextTS(4B) | Count(4B) | Min(2B) | Max(2B) | HOST(4B) | PORT(2B) | 0xFF
```

### 2.2 패킷 상세 명세

- **총 길이**: 32 바이트

| 필드 | 위치 | 크기 | 설명 |
|------|------|------|------|
| Return Code | packet[0] | 1 바이트 | 변경 요소 플래그 |
| UID | packet[1:9] | 8 바이트 | 디바이스 고유 식별자 |
| TS now | packet[9:13] | 4 바이트 | 현재 타임스탬프 |
| TS next | packet[13:17] | 4 바이트 | 다음 통신 타임스탬프 |
| Count value | packet[17:21] | 4 바이트 | 카운터 값 |
| Min speed | packet[21:23] | 2 바이트 | 최소 회전 속도 |
| Max speed | packet[23:25] | 2 바이트 | 최대 회전 속도 |
| Host address | packet[25:29] | 4 바이트 | 서버 호스트 주소 |
| Port number | packet[29:31] | 2 바이트 | 서버 포트 번호 |
| IO Flag | packet[31] | 1 바이트 | IO 제어 플래그 |

### 2.3 Return Code 비트 구조

| 비트 7 | 비트 6 | 비트 5 | 비트 4 | 비트 3 | 비트 2 | 비트 1 | 비트 0 |
|--------|--------|--------|--------|--------|--------|--------|--------|
| TS | Min, Max | - | IO | Count | Server Info | UID | Parity |

- **TS (비트 7)**: 1 = 디바이스 타임스탬프 변경, 0 = 변경 없음
- **Min, Max (비트 6)**: 1 = 이벤트 최소/최대값 변경, 0 = 변경 없음
- **IO (비트 4)**: 1 = 5초간 IO(비프음 및 LED) 활성화, 0 = 비활성화
- **Count (비트 3)**: 1 = 현재 카운트 값 변경, 0 = 변경 없음
- **Server Info (비트 2)**: 1 = 서버 정보(주소, 포트) 변경, 0 = 변경 없음
- **UID (비트 1)**: 1 = 디바이스 UID 변경, 0 = 변경 없음
- **Parity (비트 0)**: 1의 개수가 짝수가 되도록 설정

#### Return Code 예시
- **0xC0 (1100 0000)**: TS, Next TS, Min, Max 변경
- **0xC9 (1100 1001)**: TS, NextTs, Min, Max, Count 값 변경 (초기화 상태일 수 있음)

### 2.4 필드 상세 설명

#### UID (8 바이트)
- 고유 식별자: [0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F] 중 8자리 조합

#### TS now (4 바이트)
- 디바이스에 설정할 현재 타임스탬프

#### TS next (4 바이트)
- 다음 통신 타임스탬프 (TS now보다 커야 함)
- **0**: 즉시 통신
- **0xFFFFFFFF**: 통신 중지

#### Count value (4 바이트)
- 디바이스 카운터 값
- 'E1' 상태일 때 디바이스 카운터 값 설정
- 정상 상태(A0)일 때는 적용되지 않음

#### Min/Max speed of circling (각 2 바이트)
- 회전 이벤트의 최소/최대 값

#### Host address (4 바이트)
- 서버 호스트 주소, 기본값 "47.56.150.14"

#### Port number (2 바이트)
- 서버 포트 번호, 기본값 5080

#### IO Flag (1 바이트)
- **D7**: IO 플래그, 버저 및 LED 켜기/끄기
- **D[3:0]**: 디스플레이 타임아웃 시간(초), 기본값 5
- RC의 IO 비트가 1일 때만 유효
- 지정된 시간(초) 동안 IO(버저, LED)를 켠 후 꺼짐

## 3. 개발 체크리스트

- 개발 체크리스트 업데이트 필요 (dev_checklist.docx)
