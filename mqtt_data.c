#include <stdio.h>
#include "pthread.h"
#include "mqtt.h"
#include "unistd.h"
#include "cJSON.h"
#include "string.h"
#include "stdlib.h"

cJSON *tx_cjson;
cJSON *rx_cjson;
char *file_read_data;
char *file_write_data;
char *send_str;
char *device_token;
char *json_file_read();

#define HW_JS_INFO "hardware.json"
#define MEM_JS_INFO "memory.json"
char *format_send_str()
{
    tx_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(tx_cjson, "method",cJSON_CreateString("init"));
    cJSON *params_cjson;
    params_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(params_cjson,"activate_state",cJSON_CreateNumber(0));
    cJSON_AddItemToObject(params_cjson,"pub",cJSON_CreateNumber(0));
    cJSON_AddItemToObject(tx_cjson,"params",params_cjson);
    send_str = cJSON_PrintUnformatted(tx_cjson);
    printf("str is:%s\n",send_str);
    return send_str;
}

void format_hardware_str(char *device_id, char *burn_date, char *flow_id)
{
    cJSON *hw_cjson;
    hw_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(hw_cjson, "device_id",cJSON_CreateString(device_id));
    cJSON_AddItemToObject(hw_cjson,"burn_date",cJSON_CreateString(burn_date));
    cJSON_AddItemToObject(hw_cjson,"flow_id",cJSON_CreateString(flow_id));
    char *hardware_str;
    hardware_str = cJSON_PrintUnformatted(hw_cjson);
    printf("hardware_str is:%s\n",hardware_str);
    json_file_write(HW_JS_INFO, hardware_str);
}

int get_burning_info_from_file()
{
    char *rd_burn_file;
    cJSON *cjson_tmp = NULL;
    rd_burn_file = json_file_read(HW_JS_INFO);
    
    if(rd_burn_file == NULL) {
        return 0;
    }

    cjson_tmp = cJSON_Parse(rd_burn_file);
    char *device_id;
    device_id = cJSON_GetObjectItem(cjson_tmp, device_id)->valuestring;
    if(device_id == NULL) {
        return 0;
    }

    return 1;
}

char *get_device_token()
{
    return device_token;
}

char *read_token_from_hardware()
{
    char *access_token;
    char *file_read = json_file_read(HW_JS_INFO);
    cJSON *file_cjson = cJSON_Parse(file_read);
    access_token = cJSON_GetObjectItem(file_cjson,access_token)->valuestring;
    return access_token;
}

void parse_rx_data(void *pbuf)
{
    rx_cjson = NULL;
    cJSON *cjson_params = NULL;
    printf("parse_rx_data = %s\n", (unsigned char *)pbuf);
    rx_cjson = cJSON_Parse(pbuf);
    cjson_params = cJSON_GetObjectItem(rx_cjson, "params");
    char *activate_state;
    activate_state = cJSON_GetObjectItem(cjson_params,activate_state)->valuestring;
    cJSON_Delete(rx_cjson);
    return;
}

char *json_file_read(char *filename)
{
    FILE *f;
    long len;

    f = fopen(filename,"rb");
    if(f == NULL) {
        printf("file open fail.\n");
        return NULL;
    }
    fseek(f,0,SEEK_END);
    len = ftell(f);
    fseek(f,0,SEEK_SET);
    file_read_data = (char*)malloc(len+1);
    fread(file_read_data, 1, len, f);
    fclose(f);
    parse_rx_data(file_read_data);
    return file_read_data;
}

void json_file_write(char *filename, char *message)
{
    FILE *fp = fopen(filename, "w+");
    fwrite(message, strlen(message), 1, fp);
    fclose(fp);
    free(message);
}
