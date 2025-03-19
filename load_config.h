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

#include "parson.h"
#define CONFIG_FILE "config.json"

typedef struct db_tables {
  char common_user[256];
  char common_device[256];
  char data[256];
  char device[256];
  char user[256];
} db_tables;

typedef struct {
  char host[256];
  char user[256];
  char password[256];
  char db[256];
  int  port;
  struct db_tables table;
} Db;

typedef struct element{
  char key[256];
  unsigned char length;
  char format[256];
  char descrption[256];
} element;

typedef struct {
  char host[256];
  int port;
  struct element recv_packet[6];// recv_packet;
  struct element send_packet[9];// send_packet;
} Server;

typedef struct {
  char area[256];
  int tz_offset;
} Timezone;


typedef struct {
  Db DB;
  Server SERVER;
  Timezone TIMEZONE;
} Config;

Config config;

int load_config();
void display_config();