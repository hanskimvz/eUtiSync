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
#include <mysql/mysql.h>
#include "load_config.h"
#include "proc_mysql.h"

// ################# MYSQL ##########################

MYSQL_ASSOC * mysql_fetch_assoc(MYSQL_RES *res) {
    int i, j, num_fields;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    MYSQL_ASSOC *assoc_array;

    // Fetch the row
    row = mysql_fetch_row(res);
    if (!row) {
        return NULL;  // No more rows
    }

    // Get column metadata
    num_fields = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    // Allocate memory for the associative array
    assoc_array = (MYSQL_ASSOC*) malloc(num_fields * sizeof(MYSQL_ASSOC));
    if (!assoc_array) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Fill the associative array with column name -> value pairs
    for (i = 0; i < num_fields; i++) {
        // Allocate memory for column name and value
        assoc_array[i].name = (char *)malloc(strlen(fields[i].name) + 1);
        assoc_array[i].value = (char *)malloc(strlen(row[i] ? row[i] : "NULL") + 1);

        // Check for memory allocation failures
        if (!assoc_array[i].name || !assoc_array[i].value) {
            fprintf(stderr, "Memory allocation failed for name or value\n");
            // Free allocated memory for the previous entries
            for (j = 0; j < i; j++) {
                free(assoc_array[j].name);
                free(assoc_array[j].value);
            }
            free(assoc_array);
            return NULL;
        }

        // Copy the column name and value
        strcpy(assoc_array[i].name, fields[i].name);
        strcpy(assoc_array[i].value, row[i] ? row[i] : "NULL");
    }

    return assoc_array;

    // //usage
    // MYSQL_ASSOC *assoc = mysql_fetch_assoc(res);
    // if (assoc) {
    //   for (int i = 0; i < num_fields; i++) {
    //     printf("Column: %s, Value: %s\n", assoc[i].name, assoc[i].value);
    //     free(assoc[i].name);  // Free memory for name
    //     free(assoc[i].value); // Free memory for value
    //   }
    //   free(assoc);  // Free the array itself
    // }

}

int insert_device(MYSQL *conn, char db_name[256], char device_uid[8]) {
    char sq[512];  // Buffer to hold the SQL query
    int ts;
    time_t timestamp;
    char date_str[100];

    // Get the current timestamp and adjust for the timezone offset
    ts = (int)time(NULL);
    timestamp = ts + config.TIMEZONE.tz_offset;

    // Format the date and time string
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", gmtime(&timestamp));

    // Prepare the SQL query using snprintf for safety
    snprintf(sq, sizeof(sq),  
            "INSERT INTO %s.%s (device_uid, initial_access, last_access, last_timestamp) "
            "VALUES ('%s', '%s', '%s', %d)",
            db_name, config.DB.table.device, device_uid, date_str, date_str, ts);

    printf("%s: sq: %s\n", __func__, sq);
    // Execute the query
    if (mysql_query(conn, sq)) {  
        fprintf(stderr, "QUERY failed: %s\n", mysql_error(conn));
        return 1;
    }

    return 0;
}

int update_device(MYSQL *conn, char db_name[64], Table_Device device) {
    // Buffer for the SQL query
    char sq[512];  // Allocate enough space to hold the query

    // Create the SQL query using snprintf for safety
    if (device.initial_access) {
        snprintf(sq, sizeof(sq),
            "UPDATE %s.%s SET uptime=%u, battery=%d, initial_access='%s', last_access='%s', last_timestamp=%u, last_count=%u, flag='%c', status=%d WHERE device_uid='%s'",
            db_name, config.DB.table.device, device.uptime, device.battery, device.initial_access, device.last_access, device.last_timestamp, device.last_count, device.flag, device.status, device.device_uid);
    } 
    else {
        snprintf(sq, sizeof(sq),
            "UPDATE %s.%s SET uptime=%u, battery=%d, last_access='%s', last_timestamp=%u, last_count=%u, flag='%c', status=%d WHERE device_uid='%s'",
            db_name, config.DB.table.device, device.uptime, device.battery, device.last_access, device.last_timestamp, device.last_count, device.flag, device.status, device.device_uid);
    }
    // Print the query for debugging
    printf("%s: sq: %s\n", __func__, sq);

    // Execute the query
    if (mysql_query(conn, sq)) {
        fprintf(stderr, "QUERY failed: %s\n", mysql_error(conn));
        return 1;
    }

    return 0;
}

#define EXECUTE_QUERY_AND_GET_RESULT(conn, sq, res, row, device) \
  if (mysql_query(conn, sq)) { \
    fprintf(stderr, "QUERY failed: %s\n", mysql_error(conn)); \
    device.error_count = 1; \
    return device; \
  } \
  res = mysql_store_result(conn); \
  if (res == NULL) { \
    fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn)); \
    device.error_count = 2; \
    return device; \
  } \
  row = mysql_fetch_row(res); \
  if (!row) { \
    fprintf(stderr, "No data for device_uid: %s\n", device_uid); \
    device.error_count = 4; \
    mysql_free_result(res); \
    return device; \
  }



Table_Device device_info(MYSQL *conn, char device_uid[8]) {
  int i, num_fields;
  MYSQL_RES *res;
  MYSQL_FIELD *fields;
  MYSQL_ROW row;
  Table_Device device = {0};  // Initialize all fields to 0

  // Query buffer on the stack, no need for dynamic memory allocation
  char sq[256];  
  snprintf(sq, sizeof(sq), "SELECT db_name FROM %s WHERE device_uid='%s'", config.DB.table.common_device, device_uid);
  printf("%s: sq: %s\n", __func__, sq);
  EXECUTE_QUERY_AND_GET_RESULT(conn, sq, res, row, device);

 
  snprintf(device.db_name, sizeof(device.db_name), "%s", row[0] ? row[0] : "NULL");

  snprintf(sq, sizeof(sq), "SELECT * FROM %s.%s WHERE device_uid='%s'", device.db_name, config.DB.table.device, device_uid);
  printf("%s: sq: %s\n", __func__, sq);
  EXECUTE_QUERY_AND_GET_RESULT(conn, sq, res, row, device);
 
  fields = mysql_fetch_fields(res);
  num_fields = mysql_num_fields(res);  

  // Loop through fields and assign values to the appropriate device fields
  #define SET_DEVICE_INFO_INT(b, k) \
    if (strcmp(fields[i].name, b) == 0) { \
        k = row[i] ? atoi(row[i]) : 0; \
        continue; \
    }

  #define SET_DEVICE_INFO_STR(b, k) \
    if (strcmp(fields[i].name, b) == 0) { \
        snprintf(k, sizeof(k), "%s", row[i] ? row[i] : "NULL"); \
        continue; \
    }

  #define SET_DEVICE_INFO_FLAG(b, k) \
    if (strcmp(fields[i].name, b) == 0) { \
      k = row[i] ? row[i][0] : 'n'; \
      continue; \
    }


  for (i = 0; i < num_fields; i++) {
    SET_DEVICE_INFO_INT("pk",             device.pk);
    SET_DEVICE_INFO_STR("device_uid",     device.device_uid);
    SET_DEVICE_INFO_STR("meter_id",       device.meter_id);
    SET_DEVICE_INFO_INT("minimum",        device.minimum);
    SET_DEVICE_INFO_INT("maximum",        device.maximum);
    SET_DEVICE_INFO_INT("battery",        device.battery);
    SET_DEVICE_INFO_INT("uptime",         device.uptime);
    SET_DEVICE_INFO_STR("release_date",   device.release_date);
    SET_DEVICE_INFO_STR("install_date",   device.install_date);
    SET_DEVICE_INFO_STR("initial_access", device.initial_access);
    SET_DEVICE_INFO_STR("last_access",    device.last_access);
    SET_DEVICE_INFO_INT("last_timestamp", device.last_timestamp);
    SET_DEVICE_INFO_INT("last_count",     device.last_count);
    SET_DEVICE_INFO_INT("initial_count",  device.initial_count);
    SET_DEVICE_INFO_INT("ref_interval",   device.ref_interval);
    // SET_DEVICE_INFO_STR("db_name",        device.db_name);
    SET_DEVICE_INFO_INT("error_count",    device.error_count);
    SET_DEVICE_INFO_STR("param",          device.param);
    SET_DEVICE_INFO_STR("group1",         device.group1);
    SET_DEVICE_INFO_STR("group2",         device.group2);
    SET_DEVICE_INFO_STR("group3",         device.group3);
    SET_DEVICE_INFO_FLAG("flag",          device.flag);
    SET_DEVICE_INFO_INT("status",         device.status);
    SET_DEVICE_INFO_STR("server_ip",      device.server_ip);
    SET_DEVICE_INFO_INT("server_port",    device.server_port);
  }

  // Free the result set
  mysql_free_result(res);

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
    printf("Flag: %c\n", device.flag);
    printf("Status: %u\n", device.status);
    printf("Server IP: %s\n", device.server_ip);
    printf("Server Port: %u\n", device.server_port);
}


int update_counter(MYSQL *conn, char db_name[64], char device_uid[8], count counts[24]) {
  int i;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char sq[512];  // Use stack-allocated buffer
  time_t timestamp;
  struct tm *tm_info;
  char buffer[3];

  // If db_name is "none", replace it with the default database name
  if (strcmp(db_name, "none") == 0) {
    snprintf(db_name, 64, "%s", config.DB.db); // Use the explicit size of db_name (64)
  }

  // Loop through all counts
  for (i = 0; i < 24; i++) {
    if (!counts[i].ts) {
      continue;
    }

    // Check if the row already exists
    snprintf(sq, sizeof(sq), "SELECT pk FROM %s.%s WHERE uid='%s' AND timestamp=%u", db_name, config.DB.table.data, device_uid, counts[i].ts);
    printf("%s: sq: %s\n", __func__, sq);        
    if (mysql_query(conn, sq)) {
      fprintf(stderr, "QUERY failed: %s\n", mysql_error(conn));
      continue;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
      fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn));
      continue;
    }

    row = mysql_fetch_row(res);
    mysql_free_result(res);  // Free the result set after each query

    if (row) {
      // Row exists, perform the update
      timestamp = counts[i].ts + config.TIMEZONE.tz_offset;
      tm_info = gmtime(&timestamp);
      strftime(buffer, sizeof(buffer), "%V", tm_info);  // Get the week number

      // Prepare the UPDATE query
      snprintf(sq, sizeof(sq), 
            "UPDATE %s.%s SET "
            "counter_val = %u, "
            "year = %d, "
            "month = %d, "
            "day = %d, "
            "hour = %d, "
            "wday = %d, "
            "week = %d "
            "WHERE uid = '%s' AND timestamp = %u", 
            db_name, config.DB.table.data, 
            counts[i].cnt, 
            tm_info->tm_year + 1900, 
            tm_info->tm_mon + 1, 
            tm_info->tm_mday, 
            tm_info->tm_hour, 
            tm_info->tm_wday, 
            atoi(buffer), 
            device_uid, 
            counts[i].ts);
    
      printf("%s: sq: %s\n", __func__, sq);
    
      if (mysql_query(conn, sq)) {
        fprintf(stderr, "UPDATE QUERY failed: %s\n", mysql_error(conn));
    }
    } else {
    // Row doesn't exist, perform the insert
    timestamp = counts[i].ts + config.TIMEZONE.tz_offset;
    tm_info = gmtime(&timestamp);
    strftime(buffer, sizeof(buffer), "%V", tm_info);  // Get the week number

    // Prepare the INSERT query
    snprintf(sq, sizeof(sq), 
            "INSERT INTO %s.%s(uid, timestamp, counter_val, year, month, day, hour, wday, week) "
            "VALUES ('%s', %u, %u, %d, %d, %d, %d, %d, %d)", 
            db_name, config.DB.table.data, 
            device_uid, 
            counts[i].ts, 
            counts[i].cnt, 
            tm_info->tm_year + 1900, 
            tm_info->tm_mon + 1, 
            tm_info->tm_mday, 
            tm_info->tm_hour, 
            tm_info->tm_wday, 
            atoi(buffer));
    
    printf("%s: sq: %s\n", __func__, sq);
    
    if (mysql_query(conn, sq)) {
        fprintf(stderr, "INSERT QUERY failed: %s\n", mysql_error(conn));
    }
}
  }

  return 0;
}



// return code
// 7	    6	        5	    4	  3	      2	            1	      0
// TS	  Min, max			      Count	  Server_info	  UID	    Parity

// packet:
// 0   1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 28 28 29 30 31
// ret |UID           |TSC       |TSN        |COUNT      |MIN  |MAX  |HOST       |PORT | ??

int proc_db(recv_packet recp, send_packet *senp) {
  int i;
  time_t timestamp;
  MYSQL *conn;
  Table_Device device;

  // Initialize send_packet fields
  memset(senp, 0, sizeof(send_packet));  // Initialize all to 0, equivalent to your manual initialization

  conn = mysql_init(NULL);
  if (conn == NULL) {
    fprintf(stderr, "mysql_init() failed\n");
    return 1;
  }

  if (mysql_real_connect(conn, config.DB.host, config.DB.user, config.DB.password, config.DB.db, 0, NULL, 0) == NULL) {
    fprintf(stderr, "mysql_real_connect() failed\n");
    mysql_close(conn);
    return 1;
  }

  device = device_info(conn, recp.uid);
  printf("\nerror_count: %d\n", device.error_count);

  senp->tsc = (int)time(NULL);
  strncpy(senp->uid, recp.uid, 8);  

  if (device.error_count == 4) {
    printf("Not in database: %s\n", recp.uid);
    senp->ret = 0xC0; // 1100 0000
    senp->tsn = 0xFFFFFFFF;
    mysql_close(conn);
    return 2;
  }

  if (device.flag == 'n') {
    printf("deivice: %s is not activated\n", recp.uid);
    senp->ret = 0xC0; // 1100 0000
    senp->tsn = 0xFFFFFFFF;
    mysql_close(conn);
    return 3;
  }

  // default packet
  device.battery = recp.bat;
  device.uptime  = recp.uptime;

  timestamp = time(NULL) + config.TIMEZONE.tz_offset;
  strftime(device.last_access, sizeof(device.last_access), "%Y-%m-%d %H:%M:%S", gmtime(&timestamp));

  device.last_timestamp  = recp.counts[0].ts;
  device.last_count  = recp.counts[0].cnt;

  // Find the latest timestamp
  for (i = 1; i < 24; i++) {
    if (recp.counts[i].ts > device.last_timestamp) {
        device.last_timestamp = recp.counts[i].ts;
        device.last_count = recp.counts[i].cnt;
    }
  }

  senp->cnt = device.last_count;
  senp->tsn = senp->tsc + device.ref_interval;
  senp->min = device.minimum & 0xFF;
  senp->max = device.maximum & 0xFF;

    // Parse server IP address
  char *token;
  char *saveptr = NULL;  // For thread safety, use strtok_r
  token = strtok_r(device.server_ip, ".", &saveptr);
  for (i = 0; i < 4 && token != NULL; i++) {
    senp->svr[i] = atoi(token);
    token = strtok_r(NULL, ".", &saveptr);
  }

  // Check if we got 4 parts of the IP address
  if (i != 4) {
    fprintf(stderr, "Invalid server IP address format: %s\n", device.server_ip);
    // mysql_close(conn);
    return 3;
  }
  senp->prt = device.server_port;

  if (recp.eid == 0xA0) { // Normal state
    senp->ret = 0xC0; // 1100 0000

    // Update the device and counter in the database
    update_device(conn, device.db_name, device);
    printf("db_name: %s\n", device.db_name);
    update_counter(conn, device.db_name, recp.uid, recp.counts);
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
    update_device(conn, device.db_name, device);
  }

  // Close the MySQL connection
  mysql_close(conn);
  return 0;
}
