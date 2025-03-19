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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <unistd.h>
#include <pthread.h>
// #include "parson.h"
#include "load_config.h"
// #include "message.h"
// #include "proc_mysql.h"
#include "proc_mongo.h"

#define BUFFER_SIZE 256
// gcc -o server  server.c load_config.c parson.c -lpthread

int proc_message(int fd) {
  int  bytes_read, bytes_send;
  char buffer[BUFFER_SIZE];
  char* response = (char *)malloc(32);

  recv_packet recp;
  send_packet senp;
  
  // Read data from the client
  bytes_read = read(fd, buffer, BUFFER_SIZE);
  buffer[bytes_read] = '\0';  // Null-terminate the received data
  
  // proc_mongo_insert(fd, buffer, bytes_read, 1);
  // check valid packet?
  printf("recv length: %d\n", bytes_read);
  if (bytes_read < 256 || \
      !((buffer[0] & 0xFF) == 0xA0 || (buffer[0] & 0xFF) == 0xB0 || (buffer[0] & 0xFF) == 0xE0 || (buffer[0] & 0xFF) == 0xE1)) {

    if (bytes_read < 0) {
      printf("abnormal access like nmap\n");
      close(fd);
      free(response);
      return -1;
    }    
    sleep(1);
    // for Test
    if (strncmp(buffer, "test hello", 10) == 0) {
      // {0x74, 0x65, 0x73, 0x74, 0x20, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x00};
      sprintf(response, "OK, thanks");
    }
    else {
      sprintf(response, "F*U*C*K*Y*O*U");
    }
    printf("response: %s\n", response);
    bytes_send = send(fd, response, strlen(response), 0);
    proc_mongo_insert(fd, buffer, bytes_read, response, bytes_send);
    sleep(5);
    close(fd);
    free(response);
    return -1;
  }

  recp = parse_recv(buffer);
  display_recv(recp);
  proc_db(recp, &senp);
  
  response = mk_send_packet(senp);
  display_send_packet(response);
  sleep(2);
  bytes_send = send(fd, response, 32, 0);
  proc_mongo_insert(fd, buffer, bytes_read, response, bytes_send);
  sleep(5);
  close(fd);
  free(response);
  return 1;
}

int createServer() {
  int server_fd;
  struct sockaddr_in server_addr;
  
  // Create socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0) {
      perror("Socket creation failed");
      return -1;
  }

  // Set up the server address structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any address
  server_addr.sin_port = htons(config.SERVER.port);

  // Bind the socket to the address and port
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      perror("Bind failed");
      close(server_fd);
      return -1;
  }

  // Listen for incoming connections
  if (listen(server_fd, 3) < 0) {
      perror("Listen failed");
      close(server_fd);
      return -1;
  }
  printf("Server is listening on port %d...\n", config.SERVER.port);

  return server_fd;
}

// Thread function to handle client communication
void* thread_proc_message(void* arg) {
    int client_fd = *((int*)arg);
    free(arg);  // Free the memory allocated for client_fd pointer

    // Call the original proc_message function
    proc_message(client_fd);

    // Close the client socket after communication is done
    close(client_fd);

    return NULL;
}
// int mainxxx() {
//     load_config();
//     display_config();
// }
int main(){
  int server_fd = 0;
  int client_fd = 0;
  time_t timestamp;
  char date_str[20];
  // struct sockaddr_in server_addr, client_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  // char buffer[BUFFER_SIZE];

  load_config();
  server_fd = createServer();

  while (1) {
    if (server_fd < 0) {
        perror("Failed to create server ");
        sleep(5);
        server_fd = createServer();
        continue;
    }    
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("\nserver_fd:%d, client_fd:%d, client_addr:%s\n", server_fd, client_fd, inet_ntoa(client_addr.sin_addr));
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        sleep(1);
        // server_fd = createServer();
        // exit(EXIT_FAILURE);
        continue;
    }

    timestamp = time(NULL) + config.TIMEZONE.tz_offset;
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", gmtime(&timestamp));
    printf("Client connected ... %s\n", date_str); 
    // proc_message(client_fd);

    pthread_t thread_id;
    int* client_fd_ptr = malloc(sizeof(int));  // Allocate memory for client_fd
    *client_fd_ptr = client_fd;  // Store client_fd in the allocated memory

    if (pthread_create(&thread_id, NULL, thread_proc_message, client_fd_ptr) != 0) {
      perror("Failed to create thread");
      close(client_fd);     // Close client connection if thread creation fails
      free(client_fd_ptr);  // Free allocated memory
      continue;
    }
    // Detach the thread to automatically reclaim resources when it finishes
    pthread_detach(thread_id);    

  }
  close(server_fd);

  printf("Connection closed, server shutting down...\n");
  return 0;
}


// int main(){
//   load_config();
//   test_mongo_device();
//   return 0;
// }
