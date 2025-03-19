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
// #include "parson.h"
#include "load_config.h"

#define MAX_RECV_PACKET 10
#define MAX_SEND_PACKET 10

#define MONGODB

int load_config() {
    int i;
    JSON_Value *rootValue;    
    JSON_Object *rootObject;

    // Parse the JSON file
    rootValue = json_parse_file(CONFIG_FILE);
    if (rootValue == NULL) {
        fprintf(stderr, "Error parsing the configuration file.\n");
        return -1;
    }

    rootObject = json_value_get_object(rootValue);
    if (rootObject == NULL) {
        fprintf(stderr, "Error retrieving the root object.\n");
        json_value_free(rootValue);
        return -1;
    }

#ifdef MONGODB
    JSON_Object *mysqlObj = json_object_get_object(rootObject, "MONGODB");
#else
    JSON_Object *mysqlObj = json_object_get_object(rootObject, "MYSQL");
#endif
    JSON_Object *serverObj = json_object_get_object(rootObject, "SERVER");
    JSON_Object *timezoneObj = json_object_get_object(rootObject, "TIMEZONE");

    if (mysqlObj == NULL || serverObj == NULL || timezoneObj == NULL) {
        fprintf(stderr, "Error retrieving key sections in the configuration.\n");
        json_value_free(rootValue);
        return -1;
    }

    // MySQL config
    const char *mysqlHost = json_object_get_string(mysqlObj, "host");
    const char *mysqlUser = json_object_get_string(mysqlObj, "user");
    const char *mysqlPassword = json_object_get_string(mysqlObj, "password");
    const char *mysqlDb = json_object_get_string(mysqlObj, "db");
    if (mysqlHost && mysqlUser && mysqlPassword && mysqlDb) {
        strncpy(config.DB.host, mysqlHost, sizeof(config.DB.host) - 1);
        strncpy(config.DB.user, mysqlUser, sizeof(config.DB.user) - 1);
        strncpy(config.DB.password, mysqlPassword, sizeof(config.DB.password) - 1);
        strncpy(config.DB.db, mysqlDb, sizeof(config.DB.db) - 1);
        config.DB.port = (int)json_object_get_number(mysqlObj, "port");
    } 
    else {
        fprintf(stderr, "Error retrieving MySQL configuration fields.\n");
        json_value_free(rootValue);
        return -1;
    }

    // MySQL table config
    JSON_Object *tableObj = json_object_get_object(mysqlObj, "tables");
    if (tableObj) {
        strncpy(config.DB.table.common_user, json_object_get_string(tableObj, "common_user"), sizeof(config.DB.table.common_user) - 1);
        strncpy(config.DB.table.common_device, json_object_get_string(tableObj, "common_device"), sizeof(config.DB.table.common_device) - 1);
        strncpy(config.DB.table.data, json_object_get_string(tableObj, "data"), sizeof(config.DB.table.data) - 1);
        strncpy(config.DB.table.device, json_object_get_string(tableObj, "device"), sizeof(config.DB.table.device) - 1);        
        strncpy(config.DB.table.user, json_object_get_string(tableObj, "user"), sizeof(config.DB.table.user) - 1);
        // strncpy(config.DB.table.user, json_object_get_string(tableObj, "user"), sizeof(config.DB.table.user) - 1);
        // strncpy(config.DB.table.device, json_object_get_string(tableObj, "device"), sizeof(config.DB.table.device) - 1);
        // strncpy(config.DB.table.custom_data, json_object_get_string(tableObj, "custom_data"), sizeof(config.DB.table.custom_data) - 1);
        // strncpy(config.DB.table.custom_device, json_object_get_string(tableObj, "custom_device"), sizeof(config.DB.table.custom_device) - 1);
    } 
    else {
        fprintf(stderr, "Error retrieving MySQL table configuration.\n");
        json_value_free(rootValue);
        return -1;
    }

    // Server config
    const char *serverHost = json_object_get_string(serverObj, "host");
    if (serverHost) {
        strncpy(config.SERVER.host, serverHost, sizeof(config.SERVER.host) - 1);
        config.SERVER.port = (int)json_object_get_number(serverObj, "port");
    } 
    else {
        fprintf(stderr, "Error retrieving server configuration fields.\n");
        json_value_free(rootValue);
        return -1;
    }

    // Server packet arrays
    JSON_Array *recvArray = json_object_get_array(serverObj, "recv_packet");
    if (recvArray) {
        for (i = 0; i < json_array_get_count(recvArray) && i < MAX_RECV_PACKET; i++) {    
            JSON_Object *pArr = json_array_get_object(recvArray, i);
            strncpy(config.SERVER.recv_packet[i].key, json_object_get_string(pArr, "key"), sizeof(config.SERVER.recv_packet[i].key) - 1);
            config.SERVER.recv_packet[i].length = (unsigned char)json_object_get_number(pArr, "length");
            strncpy(config.SERVER.recv_packet[i].format, json_object_get_string(pArr, "format"), sizeof(config.SERVER.recv_packet[i].format) - 1);
        }
    } else {
        fprintf(stderr, "Error retrieving server recv_packet array.\n");
    }

    JSON_Array *sendArray = json_object_get_array(serverObj, "send_packet");
    if (sendArray) {
        for (i = 0; i < json_array_get_count(sendArray) && i < MAX_SEND_PACKET; i++) {    
            JSON_Object *pArr = json_array_get_object(sendArray, i);
            strncpy(config.SERVER.send_packet[i].key, json_object_get_string(pArr, "key"), sizeof(config.SERVER.send_packet[i].key) - 1);
            config.SERVER.send_packet[i].length = (unsigned char)json_object_get_number(pArr, "length");
            strncpy(config.SERVER.send_packet[i].format, json_object_get_string(pArr, "format"), sizeof(config.SERVER.send_packet[i].format) - 1);
        }
    } else {
        fprintf(stderr, "Error retrieving server send_packet array.\n");
    }

    // Timezone config
    const char *timezoneArea = json_object_get_string(timezoneObj, "area");
    if (timezoneArea) {
        strncpy(config.TIMEZONE.area, timezoneArea, sizeof(config.TIMEZONE.area) - 1);
        config.TIMEZONE.tz_offset = (int)json_object_get_number(timezoneObj, "tz_offset");
    } else {
        fprintf(stderr, "Error retrieving timezone configuration fields.\n");
        json_value_free(rootValue);
        return -1;
    }

    // Free the rootValue
    json_value_free(rootValue);
    return 0;
}

void display_config(){
    int i;
    printf("\n[DB]\n");
    printf("  host:     %s\n", config.DB.host);
    printf("  port:     %d\n", config.DB.port);
    printf("  user:     %s\n", config.DB.user);
    
    // Warning: Displaying the password is a security risk in real-world applications.
    printf("  password: %s\n", config.DB.password);  
    
    printf("  db:       %s\n", config.DB.db);
    printf("    common_device:       %s\n", config.DB.table.common_device);
    printf("    common_user:         %s\n", config.DB.table.common_user);
    printf("    user:                %s\n", config.DB.table.user);
    printf("    device:              %s\n", config.DB.table.device);
    printf("    data:                %s\n", config.DB.table.data);  

    printf("\n[SERVER]\n");
    printf("  host: %s\n", config.SERVER.host);
    printf("  port: %d\n", config.SERVER.port);
    
    // Assuming MAX_RECV_PACKET and MAX_SEND_PACKET are predefined constants
    for (i = 0; i < MAX_RECV_PACKET; i++) {
        printf("  receive[%d]: {key: %s, length: %d, format: %s}\n",
              i, config.SERVER.recv_packet[i].key, config.SERVER.recv_packet[i].length, config.SERVER.recv_packet[i].format);
    }
    
    for (i = 0; i < MAX_SEND_PACKET; i++) {
        printf("  send[%d]: {key: %s, length: %d, format: %s}\n",
              i, config.SERVER.send_packet[i].key, config.SERVER.send_packet[i].length, config.SERVER.send_packet[i].format);
    }

    printf("\n[TIMEZONE]\n");
    printf("  area:      %s\n", config.TIMEZONE.area);
    printf("  tz_offset: %d\n", config.TIMEZONE.tz_offset);
}



#if 0 
int load_config() {
  int i;
  JSON_Value *rootValue;	
  JSON_Object *rootObject; 	

  rootValue = json_parse_file(CONFIG_FILE);
  rootObject = json_value_get_object(rootValue); 	

  JSON_Object * mysqlObj    = json_object_get_object (rootObject, "MYSQL");
  JSON_Object * serverObj   = json_object_get_object (rootObject, "SERVER");
  // JSON_Object * httpObj     = json_object_get_object (rootObject, "HTTP_SERVER");
  JSON_Object * timezoneObj = json_object_get_object (rootObject, "TIMEZONE");
  
  // Mysql
  strncpy(config.DB.host, json_object_get_string(mysqlObj, "host") , sizeof(config.DB.host) - 1);
  strncpy(config.DB.user, json_object_get_string(mysqlObj, "user") , sizeof(config.DB.user) - 1);
  strncpy(config.DB.password, json_object_get_string(mysqlObj, "password") , sizeof(config.DB.password) - 1);
  strncpy(config.DB.db, json_object_get_string(mysqlObj, "db") , sizeof(config.DB.db) - 1);
  config.DB.port = (int)json_object_get_number(mysqlObj, "port");
  
  JSON_Object * tableObj  = json_object_get_object (mysqlObj, "tables");
  strncpy(config.DB.table.user, json_object_get_string(tableObj, "user") , sizeof(config.DB.table.user) - 1);
  strncpy(config.DB.table.device, json_object_get_string(tableObj, "device") , sizeof(config.DB.table.device) - 1);
  strncpy(config.DB.table.custom_data, json_object_get_string(tableObj, "custom_data") , sizeof(config.DB.table.custom_data) - 1);
  
  

  //server
  strncpy(config.SERVER.host, json_object_get_string(serverObj, "host") , sizeof(config.SERVER.host) - 1);
  config.SERVER.port =  (int)json_object_get_number(serverObj, "port");

  JSON_Object * pArr;
  JSON_Array * array;
  

  array = json_object_get_array(serverObj, "recv_packet");
  for (i = 0; i < json_array_get_count(array); i++) {	
    pArr = json_array_get_object (array, i);
    strncpy(config.SERVER.recv_packet[i].key, json_object_get_string (pArr, "key"), sizeof(config.SERVER.recv_packet[i].key) - 1);
    config.SERVER.recv_packet[i].length = (unsigned char)json_object_get_number (pArr, "length");
    strncpy(config.SERVER.recv_packet[i].format, json_object_get_string (pArr, "format"), sizeof(config.SERVER.recv_packet[i].format) - 1);
  } 	

  array = json_object_get_array(serverObj, "send_packet");
  for (i = 0; i < json_array_get_count(array); i++) {	
    pArr = json_array_get_object (array, i);
    strncpy(config.SERVER.send_packet[i].key, json_object_get_string (pArr, "key"), sizeof(config.SERVER.send_packet[i].key) - 1);
    config.SERVER.send_packet[i].length = (unsigned char)json_object_get_number (pArr, "length");
    strncpy(config.SERVER.send_packet[i].format, json_object_get_string (pArr, "format"), sizeof(config.SERVER.send_packet[i].format) - 1);
  } 	
  
  strncpy(config.TIMEZONE.area, json_object_get_string(timezoneObj, "area") , sizeof(config.TIMEZONE.area) - 1);
  config.TIMEZONE.tz_offset = (int)json_object_get_number(timezoneObj, "tz_offset");

  json_value_free(rootValue);	
  return 0;
}

void display_config(){
  int i;
  printf("\n[DB]\n");
  printf("  host:     %s\n", config.DB.host);
  printf("  port:     %d\n", config.DB.port);
  printf("  user:     %s\n", config.DB.user);
  printf("  password: %s\n", config.DB.password);
  printf("  db:       %s\n", config.DB.db);

  printf("\n[SERVER]\n");
  printf("  host: %s\n",  config.SERVER.host);
  printf("  port: %d\n",  config.SERVER.port);
  for (i=0; i<sizeof(config.SERVER.recv_packet)/sizeof(config.SERVER.recv_packet[0]); i++){
    printf ("  receive[%d]: {key: %s, length: %d, format: %s}\n", i, config.SERVER.recv_packet[i].key,  config.SERVER.recv_packet[i].length,  config.SERVER.recv_packet[i].format);
  }
  for (i=0; i<sizeof(config.SERVER.send_packet)/sizeof(config.SERVER.send_packet[0]); i++){
    printf ("  send[%d]:    {key: %s, length: %d, format: %s}\n", i, config.SERVER.send_packet[i].key,  config.SERVER.send_packet[i].length,  config.SERVER.send_packet[i].format);
  }

  printf("\n[TIMEZONE]\n");
  printf("  area:      %s\n",  config.TIMEZONE.area);
  printf("  tz_offset: %d\n",  config.TIMEZONE.tz_offset);
}

#endif