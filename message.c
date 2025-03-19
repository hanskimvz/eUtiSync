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
#include <unistd.h>
#include <time.h>
// #include <mysql/mysql.h>
// #include "parson.h"
#include "load_config.h"
#include "message.h"


char * arr_slicing(char* arr, int start, int arr_size){
  int i;
  char* slice_arr  = (char *)malloc(sizeof(char) * arr_size); 

  for(i = 0; i < arr_size; i++){
    slice_arr[i] = arr[start + i];
  }
  slice_arr[i] = '\0';
  return slice_arr;
}

recv_packet parse_recv(char *buffer) {
  int i, j;
  recv_packet recp;
  recp.eid = buffer[0] & 0xFF;
  memcpy(recp.uid, buffer + 1, 8);
  recp.uid[8] = '\0';
  for (i=0; i<24; i++) {
    recp.counts[i].ts = 0;
    recp.counts[i].cnt = 0;
    for (j=0; j<4; j++) {
      recp.counts[i].ts  |= (buffer[9 +i*8 +j]&0xFF)  << (24-j*8);
      recp.counts[i].cnt |= (buffer[13+i*8 +j]&0xFF)  << (24-j*8);
    }
  }
  recp.bat = buffer[201]&0xFF;
  recp.uptime = 0;
  for (i=0; i<4; i++) {
    recp.uptime |= (buffer[i+202]&0xFF) <<(24-8*i);
  }
  // printf("uid_in %s, %d", recp.uid, strlen(recp.uid));
  // free(buffer); do not free
  return recp;
}

void display_recv(recv_packet recp) {
  int i;
  char date_str[100];
  time_t timestamp ;
  struct tm *local_time;
  int days;

  printf ("\n === parsing info ===\n");
  printf("EID:     %02X\n", recp.eid);
  printf("UID:     %s, %zu\n", recp.uid, strlen(recp.uid));
  for (i=0; i<24; i++) {
    timestamp =  recp.counts[i].ts + config.TIMEZONE.tz_offset;
    local_time = gmtime(&timestamp);
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", local_time);
    printf("COUNT:   %2d,  %u=%7u, %s\n", i, recp.counts[i].ts, recp.counts[i].cnt, date_str);
    
  }
  printf("BATERRY: %d\n", recp.bat);
  timestamp =  recp.uptime;
  local_time = gmtime(&timestamp);
  strftime(date_str, sizeof(date_str), "%H:%M:%S", local_time);
  days = recp.uptime/(3600*24);
  printf("UPTIME:  %u, %d days %s\n", recp.uptime, days, date_str);
}

// Function to pack a 32-bit integer into a buffer (big-endian)
void pack_uint32(char* buffer, int offset, unsigned int value) {
  int i;
  for ( i = 0; i < 4; i++) {
      buffer[offset + i] = (value >> (24 - i * 8)) & 0xFF;
  }
}

// Function to pack a 16-bit integer into a buffer (big-endian)
void pack_uint16(char* buffer, int offset, unsigned short value) {
  int i;
  for ( i = 0; i < 2; i++) {
      buffer[offset + i] = (value >> (8 - i * 8)) & 0xFF;
  }
}

char* mk_send_packet(send_packet senp){
    // Allocate space for the packet
    char* s_packet = malloc(32 * sizeof(char)); 

    // Pack each field into the packet using sub-functions
    s_packet[0] = senp.ret;                  // Pack 1 byte (ret)
    memcpy(s_packet + 1, senp.uid, 8);       // Pack 8 bytes (UID)
    pack_uint32(s_packet, 9, senp.tsc);      // Pack 4 bytes (tsc)
    pack_uint32(s_packet, 13, senp.tsn);     // Pack 4 bytes (tsn)
    pack_uint32(s_packet, 17, senp.cnt);     // Pack 4 bytes (cnt)
    pack_uint16(s_packet, 21, senp.min);     // Pack 2 bytes (min)
    pack_uint16(s_packet, 23, senp.max);     // Pack 2 bytes (max)
    memcpy(s_packet + 25, senp.svr, 4);      // Pack 4 bytes (server ip)
    pack_uint16(s_packet, 29, senp.prt);     // Pack 2 bytes (port)
    s_packet[31] = 0xFF;
  
  return s_packet;
}

void display_send_packet(char *buffer) {
  // parse packet and display...
  int i, temp;
  char date_str[100];
  time_t timestamp ;
  struct tm *local_time;  

  printf("\n\n=== send packet info===\n");
  for (i=0; i<32; i++) {
    printf("%d:%02x, ", i, buffer[i]&0xFF);
  }
  printf("\n");
  printf("RET: %d(0x%02X)\n", buffer[0] & 0xFF,buffer[0] & 0xFF);
  printf("UID: %s\n", arr_slicing(buffer, 1, 8));

  temp = 0;
  for (i=0; i<4; i++) {
    temp |= (buffer[9 +i]&0xFF)  << (24-i*8);
  }
  timestamp =  temp + config.TIMEZONE.tz_offset;
  local_time = gmtime(&timestamp);
  strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", local_time);  
  printf("TSC: %u  %s\n", temp, date_str);

  temp = 0;
  for (i=0; i<4; i++) {
    temp |= (buffer[13 +i]&0xFF)  << (24-i*8);
  }
  timestamp =  temp + config.TIMEZONE.tz_offset;
  local_time = gmtime(&timestamp);
  strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", local_time);    
  printf("TSN: %u  %s\n", temp, date_str);

  temp = 0;
  for (i=0; i<4; i++) {
    temp |= (buffer[17 +i]&0xFF)  << (24-i*8);
  }
  printf("CNT: %u\n", temp);

  temp = 0;
  for (i=0; i<2; i++) {
    temp |= (buffer[21 +i]&0xFF)  << (8-i*8);
  }
  printf("MIN: %d\n", temp);

  temp = 0;
  for (i=0; i<2; i++) {
    temp |= (buffer[23 +i]&0xFF)  << (8-i*8);
  }
  printf("MAX: %d\n", temp);

  printf("SVR: %u.%u.%u.%u\n", buffer[25]&0xFF, buffer[26]&0xFF, buffer[27]&0xFF, buffer[28]&0xFF);
  printf("PRT: %u\n", (buffer[29]&0xFF) << 8 | (buffer[30]&0xFF));
  // free(buffer); do not free
}
