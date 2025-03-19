/****************************************************************************
Copyright (c) 2024, Hans kim(hanskimvz@gmail.com)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/**
 * serializer.c
 * 
 * 제조 과정에서 디바이스 시리얼 번호를 설정하는 프로그램
 * 초기 UID가 "FFFFFFFF"인 디바이스가 서버에 5080 포트로 접속하면
 * 명령줄에서 지정한 시리얼 번호를 할당해주는 서버 프로그램
 * 사용법: ./serializer [새로운_USN]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../message.h"
#include "../load_config.h"

#define BUFFER_SIZE 256
#define SERIAL_PORT 5080
#define DEFAULT_UID "FFFFFFFF"
#define SERVER_IP "140.245.72.44"  // 메인 서버 IP
#define SERVER_PORT 5004           // 메인 서버 포트

// 전역 변수
char new_serial[9] = {0}; // 새로 할당할 시리얼 번호 (8자리 + NULL 종료자)

// 함수 선언
int createServer();
int process_client(int client_fd);
int check_device_uid(const char *uid);

/**
 * 디바이스 UID가 기본값인지 확인하는 함수
 */
int check_device_uid(const char *uid) {
    return strcmp(uid, DEFAULT_UID) == 0;
}

/**
 * 클라이언트 요청 처리 함수
 */
int process_client(int client_fd) {
    int bytes_read, bytes_sent;
    char buffer[BUFFER_SIZE];
    char *response;
    recv_packet recp;
    send_packet senp;
    char temp_ip[32];
    
    // 디바이스로부터 데이터 읽기
    printf("패킷 수신 대기 중...\n");
    bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    
    if (bytes_read < 256) {
        printf("잘못된 패킷 크기: %d 바이트\n", bytes_read);
        return -1;
    }
    
    printf("256바이트 패킷 수신 완료\n");
    
    // 패킷 유효성 검사
    if (!((buffer[0] & 0xFF) == 0xA0 || (buffer[0] & 0xFF) == 0xB0 || 
          (buffer[0] & 0xFF) == 0xE0 || (buffer[0] & 0xFF) == 0xE1)) {
        printf("잘못된 패킷 형식: 이벤트 ID 오류\n");
        return -1;
    }
    
    // 패킷 파싱
    recp = parse_recv(buffer);
    display_recv(recp);
    
    // UID 확인
    if (check_device_uid(recp.uid)) {
        printf("기본 UID 디바이스 감지: %s -> 새 USN으로 변경: %s\n", recp.uid, new_serial);
        
        // 응답 패킷 생성
        senp.ret = 0xC6; // UID 및 서버 정보 변경 플래그
        memcpy(senp.uid, new_serial, 8); // strncpy 대신 memcpy 사용
        senp.tsc = 0;    // 0으로 설정 (prod_server.c와 동일하게)
        senp.tsn = 0;  // 즉시 통신
        senp.cnt = 0;
        senp.min = 7200;
        senp.max = 0;
        
        // 서버 IP 및 포트 설정
        strncpy(temp_ip, SERVER_IP, sizeof(temp_ip)-1);
        temp_ip[sizeof(temp_ip)-1] = '\0';
        
        char *token;
        char *saveptr = NULL;
        int i = 0;
        
        token = strtok_r(temp_ip, ".", &saveptr);
        for (i = 0; i < 4 && token != NULL; i++) {
            senp.svr[i] = atoi(token);
            token = strtok_r(NULL, ".", &saveptr);
        }
        
        senp.prt = SERVER_PORT;
        
        // 응답 패킷 생성
        response = mk_send_packet(senp);
        
        // 응답 전송
        printf("새 USN 전송: %s\n", new_serial);
        display_send_packet(response);
        
        printf("응답 패킷 전송 중...\n");
        sleep(2);
        bytes_sent = send(client_fd, response, 32, 0);
        
        if (bytes_sent != 32) {
            printf("응답 전송 실패: %d 바이트\n", bytes_sent);
            free(response);
            return -1;
        }
        
        printf("USN 변경 성공\n");
        free(response);
        sleep(5);
        return 1;
    } else {
        printf("기본 UID가 아닌 디바이스: %s (USN 변경 불필요)\n", recp.uid);
        return 0;
    }
}

/**
 * 서버 생성 함수
 */
int createServer() {
    int server_fd;
    struct sockaddr_in server_addr;
    
    // 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // 소켓 옵션 설정 (주소 재사용)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt 실패");
        close(server_fd);
        return -1;
    }
    
    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERIAL_PORT);
    
    // 소켓 바인딩
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }
    
    // 연결 대기
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }
    
    printf("시리얼 할당 서버가 %d 포트에서 실행 중입니다...\n", SERIAL_PORT);
    
    return server_fd;
}

/**
 * 메인 함수
 */
int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int result;
    
    // 명령줄 인수 확인
    if (argc != 2) {
        printf("사용법: %s [새로운_USN]\n", argv[0]);
        printf("예시: %s FB6E88BE\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // 시리얼 번호 길이 검증
    if (strlen(argv[1]) != 8) {
        printf("오류: USN은 정확히 8자여야 합니다!\n");
        return EXIT_FAILURE;
    }
    
    // 새 시리얼 번호 저장
    strncpy(new_serial, argv[1], 8);
    new_serial[8] = '\0';
    
    // 설정 로드
    load_config();
    
    // 서버 생성
    server_fd = createServer();
    if (server_fd < 0) {
        return EXIT_FAILURE;
    }
    
    printf("기본 UID \"%s\"인 디바이스 접속 시 새 USN \"%s\"로 변경합니다...\n", 
           DEFAULT_UID, new_serial);
    
    // 클라이언트 연결 수락
    printf("디바이스 연결 대기 중...\n");
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        return EXIT_FAILURE;
    }
    
    printf("디바이스 연결됨: %s\n", inet_ntoa(client_addr.sin_addr));
    
    // 클라이언트 요청 처리
    result = process_client(client_fd);
    
    // 연결 종료
    close(client_fd);
    close(server_fd);
    
    if (result == 1) {
        printf("작업 완료: USN 변경 성공\n");
    } else if (result == 0) {
        printf("작업 완료: USN 변경 불필요\n");
    } else {
        printf("작업 실패: 오류 발생\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
