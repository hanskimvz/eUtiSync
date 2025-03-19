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

#include <libmongoc-1.0/mongoc.h>
#include <libbson-1.0/bson.h>
#include "message.h"

typedef struct {
  unsigned int  pk;
  char device_uid[128];
  char meter_id[128];
  char firmware_version[128];
  unsigned int minimum;
  unsigned int maximum;
  unsigned int uptime;
  unsigned int battery;
  char release_date[128];
  char install_date[128];
  char initial_access[128];
  char last_access[128];
  unsigned int last_timestamp;
  unsigned int initial_count;
  unsigned int last_count;
  unsigned int ref_interval;
  char db_name[60];
  unsigned int error_count;
  char param[256];
  char server_ip[32];
  unsigned int server_port;
  char user_id[64];
  char installer_id[64];
  char group1[128];
  char group2[128];
  char group3[128];
  char comment[256];
  unsigned int status;
  char flag;
} Table_Device;


int proc_db(recv_packet recp, send_packet *senp);
// int db_test();


