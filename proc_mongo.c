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
#include <libmongoc-1.0/mongoc.h>
#include <libbson-1.0/bson.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "load_config.h"
#include "proc_mongo.h"

// gcc -o procmongo proc_mongo.c load_config.c parson.c $(pkg-config --cflags --libs libmongoc-1.0)

mongoc_client_t* dbMaster() {
  mongoc_client_t* client;
  char uri[512]; // 더 큰 버퍼 크기
  int result;
  
  // snprintf는 버퍼 오버플로우를 방지하고 쓰여진 문자 수를 반환합니다
  result = snprintf(uri, sizeof(uri), "mongodb://%s:%s@%s:%d", 
    config.DB.user,
    config.DB.password,
    config.DB.host, 
    config.DB.port
  );
  
  // 버퍼가 충분한지 확인
  if (result < 0 || result >= sizeof(uri)) {
    fprintf(stderr, "MongoDB URI 생성 실패: 버퍼 크기 초과\n");
    return NULL;
  }
  
  client = mongoc_client_new(uri);
  if (!client) {
    fprintf(stderr, "MongoDB 클라이언트 생성 실패\n");
    return NULL;
  }
  
  return client;
}


char* get_db_name(mongoc_client_t* client, char usn[64]) {
    mongoc_collection_t* collection = mongoc_client_get_collection(client, config.DB.db, config.DB.table.common_device);
   
    bson_t* query = bson_new();
    BSON_APPEND_UTF8(query, "device_uid", usn);
    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t* doc;
    char* db_name = NULL;
    db_name = malloc(64);
    
    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        if (bson_iter_init(&iter, doc)) {
            while (bson_iter_next(&iter)) {
                const char* key = bson_iter_key(&iter);
                if (strcmp(key, "db_name") == 0) {
                    const char* temp = bson_iter_utf8(&iter, NULL);
                    db_name = strdup(temp);
                }
            }
        }
    }
    else {
      // printf("Not found device_uid: %s\n", usn);
      strncpy(db_name, "NULL", 64);
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return db_name; // 호출자가 free() 해야 함
}

bool is_valid_utf8(const char* str) {
    const unsigned char* bytes = (const unsigned char*)str;
    while (*bytes) {
        if ((bytes[0] & 0x80) == 0) {          // 1 byte
            bytes += 1;
        } else if ((bytes[0] & 0xE0) == 0xC0) { // 2 bytes
            if ((bytes[1] & 0xC0) != 0x80) return false;
            bytes += 2;
        } else if ((bytes[0] & 0xF0) == 0xE0) { // 3 bytes
            if ((bytes[1] & 0xC0) != 0x80 || 
                (bytes[2] & 0xC0) != 0x80) return false;
            bytes += 3;
        } else if ((bytes[0] & 0xF8) == 0xF0) { // 4 bytes
            if ((bytes[1] & 0xC0) != 0x80 || 
                (bytes[2] & 0xC0) != 0x80 ||
                (bytes[3] & 0xC0) != 0x80) return false;
            bytes += 4;
        } else {
            return false;
        }
    }
    return true;
}

int proc_mongo_insert(int fd, char *recv_buffer, int recv_length, char *send_buffer, int send_length) {
  printf("proc_mongo_insert\n");
  time_t timestamp;
  char date_str[32];
  
  timestamp = time(NULL) + config.TIMEZONE.tz_offset;
  strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", gmtime(&timestamp));  

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  getpeername(fd, (struct sockaddr*)&client_addr, &addr_len);
  printf("ip: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

  bson_error_t error;
  bool success;
  
  mongoc_init();
  mongoc_client_t* client = dbMaster();

  mongoc_collection_t* collection = mongoc_client_get_collection(client, config.DB.db, "misc_data");


  bson_t* doc = bson_new();
  BSON_APPEND_UTF8(doc, "ip", inet_ntoa(client_addr.sin_addr));
  BSON_APPEND_INT32(doc, "port", (int)config.SERVER.port);
  BSON_APPEND_UTF8(doc, "datetime", date_str);
  BSON_APPEND_INT32(doc, "recv_len", recv_length);
  BSON_APPEND_BINARY(doc, "recv", BSON_SUBTYPE_BINARY, (const uint8_t*)recv_buffer, recv_length);
  BSON_APPEND_INT32(doc, "send_len", send_length);
  BSON_APPEND_BINARY(doc, "send", BSON_SUBTYPE_BINARY, (const uint8_t*)send_buffer, send_length);
  // bson_append_binary(doc, "data", -1, BSON_SUBTYPE_BINARY, (const uint8_t*)buffer, strlen(buffer));
  

  success = mongoc_collection_insert_one(collection, doc, NULL, NULL, &error);
  if(!success) {
    fprintf(stderr, "Insert Error: %s\n", error.message);
  }
  bson_destroy(doc);

  mongoc_collection_destroy(collection);
  mongoc_client_destroy(client);
  mongoc_cleanup();
  return 0;
}

Table_Device device_info(mongoc_client_t* client, char usn[64]){
  Table_Device device;
  strncpy(device.device_uid, usn, 8);
  device.device_uid[8] = '\0';

  char* db_name = get_db_name(client, usn);
  strncpy(device.db_name, db_name, 64);
  free(db_name);

  if (strcmp(device.db_name, "NULL") == 0) {
    device.error_count = 4;
    return device;
  }

  device.last_timestamp = 0;
  device.last_count = 0;
  device.initial_count = 0;
  device.ref_interval = 0;
  device.error_count = 0;
  device.status = 0;


  #define SET_DEVICE_INFO_INT(b, k) \
    if (strcmp(key, b) == 0) { \
        k = bson_iter_int32(&iter); \
        continue; \
    }
  #define SET_DEVICE_INFO_STR(b, k) \
    if (strcmp(key, b) == 0) { \
        snprintf(k, sizeof(k), "%s", bson_iter_utf8(&iter, NULL)); \
        continue; \
    }
  #define SET_DEVICE_INFO_FLAG(b, k) \
    if (strcmp(key, b) == 0) { \
      k = bson_iter_bool(&iter) ? 1: 0; \
      continue; \
    }

  mongoc_collection_t* collection = mongoc_client_get_collection(client, device.db_name, config.DB.table.device);
  bson_t* query = bson_new();
  BSON_APPEND_UTF8(query, "device_uid", usn);
  mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
  const bson_t* doc;
  if (mongoc_cursor_next(cursor, &doc)) {
      bson_iter_t iter;
      if (bson_iter_init(&iter, doc)) {
          while (bson_iter_next(&iter)) {
              const char* key = bson_iter_key(&iter);
              SET_DEVICE_INFO_STR("meter_id", device.meter_id);
              SET_DEVICE_INFO_INT("minimum", device.minimum);
              SET_DEVICE_INFO_INT("maximum", device.maximum);
              SET_DEVICE_INFO_INT("battery", device.battery);
              SET_DEVICE_INFO_INT("uptime", device.uptime);
              SET_DEVICE_INFO_STR("initial_access", device.initial_access);
              SET_DEVICE_INFO_STR("last_access", device.last_access);
              SET_DEVICE_INFO_INT("last_timestamp", device.last_timestamp);
              SET_DEVICE_INFO_INT("initial_count", device.initial_count);
              SET_DEVICE_INFO_INT("last_count", device.last_count);
              SET_DEVICE_INFO_INT("ref_interval", device.ref_interval);
              SET_DEVICE_INFO_INT("error_count", device.error_count);
              SET_DEVICE_INFO_STR("param", device.param);
              SET_DEVICE_INFO_STR("group1", device.group1);
              SET_DEVICE_INFO_STR("group2", device.group2);
              SET_DEVICE_INFO_STR("group3", device.group3);
              SET_DEVICE_INFO_FLAG("flag", device.flag);
              SET_DEVICE_INFO_INT("status", device.status);
              SET_DEVICE_INFO_STR("server_ip", device.server_ip);
              SET_DEVICE_INFO_INT("server_port", device.server_port);
              SET_DEVICE_INFO_STR("release_date", device.release_date);
              SET_DEVICE_INFO_STR("install_date", device.install_date);
          }
      }
  }
  
  bson_destroy(query);
  mongoc_cursor_destroy(cursor);
  mongoc_collection_destroy(collection);
  
  return device;
}

void print_device_info(Table_Device device) {
  printf("PK: %u\n", device.pk);
  printf("Device UID: %s\n", device.device_uid);
  printf("Meter ID: %s\n", device.meter_id);
  printf("Minimum: %u\n", device.minimum);
  printf("Maximum: %u\n", device.maximum);
  printf("Uptime: %u\n", device.uptime);
  printf("Battery: %u\n", device.battery);
  printf("Initial Access: %s\n", device.initial_access);
  printf("Last Access: %s\n", device.last_access);
  printf("Last Timestamp: %u\n", device.last_timestamp);
  printf("Initial Count: %u\n", device.initial_count);
  printf("Last Count: %u\n", device.last_count);
  printf("Ref Interval: %u\n", device.ref_interval);
  printf("DB Name: %s\n", device.db_name);
  printf("Error Count: %u\n", device.error_count);
  printf("Param: %s\n", device.param);
  printf("Group1: %s\n", device.group1);
  printf("Group2: %s\n", device.group2);
  printf("Group3: %s\n", device.group3);
  printf("Flag: %d\n", device.flag);
  printf("Status: %u\n", device.status);
  printf("Server IP: %s\n", device.server_ip);
  printf("Server Port: %u\n", device.server_port);
  printf("Release Date: %s\n", device.release_date);
  printf("Install Date: %s\n", device.install_date);
}

int update_device(mongoc_client_t* client, char db_name[64], Table_Device device) {
  mongoc_collection_t* collection = mongoc_client_get_collection(client, db_name, config.DB.table.device);
  
  bson_t* query;
  bson_t* update;
  bson_t* set;
  // bson_t* opts;
  bson_error_t error;
  bool success;
  
  query = bson_new();
  update = bson_new();
  set = bson_new();
    
  BSON_APPEND_UTF8(query, "device_uid", device.device_uid);
  // BSON_APPEND_UTF8(set, "meter_id", device.meter_id);
  BSON_APPEND_UTF8(set, "last_access", device.last_access);
  BSON_APPEND_UTF8(set, "initial_access", device.initial_access);
  BSON_APPEND_INT32(set, "uptime", device.uptime);
  BSON_APPEND_INT32(set, "battery", device.battery);
  BSON_APPEND_INT32(set, "last_timestamp", device.last_timestamp);
  BSON_APPEND_INT32(set, "last_count", device.last_count);
  // BSON_APPEND_INT32(set, "initial_count", device.initial_count);
  // BSON_APPEND_INT32(set, "ref_interval", device.ref_interval);
  // BSON_APPEND_INT32(set, "error_count", device.error_count);
  // BSON_APPEND_INT32(set, "status", device.status);
  // BSON_APPEND_UTF8(set, "server_ip", device.server_ip);
  // BSON_APPEND_INT32(set, "server_port", device.server_port);
  // BSON_APPEND_UTF8(set, "release_date", device.release_date);
  // BSON_APPEND_UTF8(set, "install_date", device.install_date);
  // BSON_APPEND_UTF8(set, "group1", device.group1);
  // BSON_APPEND_UTF8(set, "group2", device.group2);
  // BSON_APPEND_UTF8(set, "group3", device.group3);
  // BSON_APPEND_UTF8(set, "param", device.param); 
  // BSON_APPEND_BOOL(set, "flag", device.flag);

  BSON_APPEND_DOCUMENT(update, "$set", set);
  // BSON_APPEND_BOOL(opts, "upsert", true);
  // success = mongoc_collection_update_one(collection, query, update, opts, NULL, &error);
  success = mongoc_collection_update_one(collection, query, update, NULL, NULL, &error);
    
  if (!success) {
      fprintf(stderr, "Error: %s\n", error.message);
  } else {
      printf("Successfully updated device\n");
  }
  
  bson_destroy(query);
  bson_destroy(update);
  bson_destroy(set);
  // bson_destroy(opts);
  mongoc_collection_destroy(collection);

  return success;
}

int update_counter(mongoc_client_t* client, char db_name[64], char device_uid[8], count counts[24]) {
  int i;
  time_t timestamp;
  struct tm *tm_info;
  char buffer[3];

  bson_error_t error;
  bool success;

  // If db_name is "none", replace it with the default database name
  if (strcmp(db_name, "none") == 0) {
    strncpy(db_name, config.DB.db, 63); // 64 - 1 to ensure null termination
    db_name[63] = '\0'; // Ensure null termination
  }

  mongoc_collection_t* collection = mongoc_client_get_collection(client, db_name, config.DB.table.data);

  // Loop through all counts
  for (i = 0; i < 24; i++) {
    if (!counts[i].ts) {
      continue;
    }
    bson_t* query = bson_new();
    BSON_APPEND_UTF8(query, "device_uid", device_uid);
    BSON_APPEND_INT32(query, "timestamp", counts[i].ts);

    // 해당 timestamp에 데이터가 있는지 확인
    // mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    int count = mongoc_collection_count_documents(collection, query, NULL, NULL, NULL, &error);
    // printf("count: %d\n", count);

    if(!count) {
      // Row doesn't exist, perform the insert  
      // printf("Row doesn't exist, perform the insert: %d:%d\n", counts[i].ts, counts[i].cnt);
      timestamp = counts[i].ts + config.TIMEZONE.tz_offset;
      tm_info = gmtime(&timestamp);
      strftime(buffer, sizeof(buffer), "%V", tm_info);  // Get the week number    
      
      bson_t* doc = bson_new();
      BSON_APPEND_UTF8(doc, "device_uid", device_uid);
      BSON_APPEND_INT32(doc, "timestamp", counts[i].ts);
      BSON_APPEND_INT32(doc, "counter_val", counts[i].cnt);

      BSON_APPEND_INT32(doc, "year", tm_info->tm_year + 1900);
      BSON_APPEND_INT32(doc, "month", tm_info->tm_mon + 1);
      BSON_APPEND_INT32(doc, "day", tm_info->tm_mday);
      BSON_APPEND_INT32(doc, "hour", tm_info->tm_hour);
      BSON_APPEND_INT32(doc, "wday", tm_info->tm_wday);
      BSON_APPEND_INT32(doc, "week", atoi(buffer));

      success = mongoc_collection_insert_one(collection, doc, NULL, NULL, &error);
      if(!success) {
        fprintf(stderr, "Insert Error: %s\n", error.message);
      }
      bson_destroy(doc);

    }
    bson_destroy(query);
    // bson_destroy(doc);
    // mongoc_cursor_destroy(cursor);
  }

  mongoc_collection_destroy(collection);
  return 0;
}

int verifyIPv4(char *ip, int svr[4]){
  // Parse server IP address
  int i;
  char *token;
  char *saveptr = NULL;
  char temp_ip[32];
  strncpy(temp_ip, ip, sizeof(temp_ip)-1);
  temp_ip[sizeof(temp_ip)-1] = '\0';

  token = strtok_r(temp_ip, ".", &saveptr);
  for (i = 0; i < 4 && token != NULL; i++) {
    svr[i] = atoi(token);
    token = strtok_r(NULL, ".", &saveptr);
  }

  if (i != 4) {
    fprintf(stderr, "Invalid server IP address format: %s\n", ip);
    return -1;
  }
  return 0;
}


int proc_db(recv_packet recp, send_packet *senp) {
  int i;
  time_t timestamp;
  Table_Device device;

  // Initialize send_packet fields
  memset(senp, 0, sizeof(send_packet));  // Initialize all to 0, equivalent to your manual initialization

  mongoc_init();
  mongoc_client_t* client = dbMaster();

  device = device_info(client, recp.uid);
  print_device_info(device);
  printf("\nerror_count: %d\n", device.error_count);
  printf("\nflag: %d\n", device.flag);

  senp->tsc = (int)time(NULL);
  strncpy(senp->uid, recp.uid, 8);  

  if (device.error_count == 4) {
    printf("Not in database: %s\n", recp.uid);
    senp->ret = 0xC0; // 1100 0000
    senp->tsn = 0xFFFFFFFF;
    mongoc_client_destroy(client);
    mongoc_cleanup();
    return 2;
  }

  if (!device.flag || device.flag == 'n') {
    printf("deivice: %s is not activated\n", recp.uid);
    senp->ret = 0xC0; // 1100 0000
    senp->tsn = 0xFFFFFFFF;
    mongoc_client_destroy(client);
    mongoc_cleanup();
    return 3;
  }

  device.battery = recp.bat;
  device.uptime  = recp.uptime;

  timestamp = time(NULL) + config.TIMEZONE.tz_offset;
  strftime(device.last_access, sizeof(device.last_access), "%Y-%m-%d %H:%M:%S", gmtime(&timestamp));

  // Find the latest timestamp
  device.last_timestamp  = recp.counts[0].ts;
  device.last_count  = recp.counts[0].cnt;
  for (i = 1; i < 24; i++) {
    if (recp.counts[i].ts > device.last_timestamp) {
        device.last_timestamp = recp.counts[i].ts;
        device.last_count = recp.counts[i].cnt;
    }
  }
  senp->cnt = device.last_count;
  senp->tsn = senp->tsc + device.ref_interval;
  senp->min = device.minimum & 0xFFFF;
  senp->max = device.maximum & 0xFFFF;

  // Parse server IP address
  char *token;
  char *saveptr = NULL;
  char temp_ip[32];
  strncpy(temp_ip, device.server_ip, sizeof(temp_ip)-1);
  temp_ip[sizeof(temp_ip)-1] = '\0';

  token = strtok_r(temp_ip, ".", &saveptr);
  for (i = 0; i < 4 && token != NULL; i++) {
    senp->svr[i] = atoi(token);
    token = strtok_r(NULL, ".", &saveptr);
  }

  if (i != 4) {
    fprintf(stderr, "Invalid server IP address format: %s\n", device.server_ip);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    return 3;
  }
  senp->prt = device.server_port;

  if (recp.eid == 0xA0) { // Normal state
    senp->ret = 0xC0; // 1100 0000

    // Update the device and counter in the database
    print_device_info(device);
    update_device(client, device.db_name, device);
    update_counter(client, device.db_name, recp.uid, recp.counts);
  }

  else if (recp.eid == 0xE0) { // Event: Set an event alarm
      // Handle event case here
  }
  
  // Initialize meter
  else if (recp.eid == 0xE1) { 
    senp->ret = 0xC9; // 1100 1001
    senp->cnt = device.initial_count;
    senp->tsn = 0;

    strncpy(device.initial_access, device.last_access, sizeof(device.initial_access) - 1);
    device.initial_access[sizeof(device.initial_access) - 1] = '\0';  // Ensure null termination
    device.last_count = device.initial_count;

    print_device_info(device);
    update_device(client, device.db_name, device);
  }

  mongoc_client_destroy(client);
  mongoc_cleanup();
  
  return 0;  

}

void test_mongo_device(){
  mongoc_init();
  mongoc_client_t* client = dbMaster();
  Table_Device device = device_info(client, "FCFB0515");
  print_device_info(device);
  mongoc_client_destroy(client);
  mongoc_cleanup();
}

#if 0
// 메인 함수 예제
int main() {
  load_config();
  // display_config();
  recv_packet recp;
  send_packet senp;
  // A149490D
  // A0775932
  recp.eid = 0xA0;
  recp.counts[0].ts = 1725321600;
  recp.counts[0].cnt = 1000;

  int i;
  for (i = 1; i < 24; i++) {
    recp.counts[i].ts = recp.counts[0].ts + 3600*i;
    recp.counts[i].cnt = recp.counts[0].cnt + i*2;
  }

  strncpy(recp.uid, "A0775932", 8);
  proc_db(recp, &senp);

  // strncpy(recp.uid, "A149490D", 8);
  // proc_db(recp, &senp);

  return 0;
}
#endif
