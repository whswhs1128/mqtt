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
#define PRODUCT_HEAD "CEC0-AS0000-"

char *formate_productid()
{
    char *rd_burn_file;
    cJSON *cjson_tmp = NULL;
    rd_burn_file = json_file_read(HW_JS_INFO);

    cjson_tmp = cJSON_Parse(rd_burn_file);
    char *burn_date;
    burn_date = malloc(9);
    //char burn_date[9];
    if(cJSON_GetObjectItem(cjson_tmp, "burn_date") == NULL) return NULL;
    burn_date = cJSON_GetObjectItem(cjson_tmp, "burn_date")->valuestring;
    char *burn_date_short;
    burn_date_short = malloc(7);
    burn_date_short = &burn_date[2];

    char *flow_id;
    flow_id = malloc(4);
    char *flow_id_long;
    flow_id_long = malloc(7);
    if(cJSON_GetObjectItem(cjson_tmp, "flow_id") == NULL) return NULL;
    flow_id = cJSON_GetObjectItem(cjson_tmp, "flow_id")->valuestring;
    switch(strlen(flow_id)) {
        case 1:
            sprintf(flow_id_long, "00000%s",flow_id);
            break;
        case 2:
            sprintf(flow_id_long, "0000%s",flow_id);
            break;
        case 3:
            sprintf(flow_id_long, "000%s",flow_id);
            break;
        case 4:
            sprintf(flow_id_long, "00%s",flow_id);
            break;
        case 5:
            sprintf(flow_id_long, "0%s",flow_id);
            break;
    }

    char *product_id;
    product_id = malloc(26);
    strcpy(product_id, PRODUCT_HEAD);
    strcat(product_id, burn_date_short);
    strcat(product_id, "-");
    strcat(product_id, flow_id_long);
    printf("formate_productid is %s\n",product_id);
    return product_id;

}


int get_activate_state()
{
    char *rd_file;
    cJSON *cjson_tmp = NULL;
    rd_file = json_file_read(MEM_JS_INFO);

    cjson_tmp = cJSON_Parse(rd_file);
    // char *burn_date;
    // burn_date = malloc(9);
    char *activate_state;
    if(cJSON_GetObjectItem(cjson_tmp, "activate_state") == NULL) return 0;
    activate_state = cJSON_GetObjectItem(cjson_tmp, "activate_state")->valuestring;
    return 1;
}

char *format_first_init()
{
    char *product_id;
    product_id = malloc(26);
    product_id = formate_productid();
    
    tx_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(tx_cjson, "method",cJSON_CreateString("init"));
    cJSON *params_cjson;
    params_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(params_cjson,"activate_state",cJSON_CreateNumber(0));
    cJSON_AddItemToObject(params_cjson,"productId",cJSON_CreateString(product_id));
    cJSON_AddItemToObject(tx_cjson,"params",params_cjson);
    send_str = cJSON_PrintUnformatted(tx_cjson);
    printf("str is:%s\n",send_str);
    return send_str;
}

char *format_heartbeat_str()
{
    char *product_id;
    product_id = malloc(26);
    product_id = formate_productid();
    
    tx_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(tx_cjson, "method",cJSON_CreateString("init-finished"));
    cJSON *params_cjson;
    params_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(params_cjson,"activate_state",cJSON_CreateNumber(1));
    cJSON_AddItemToObject(params_cjson,"productId",cJSON_CreateString(product_id));
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
    printf("rd_burn_file is %s\n",rd_burn_file);
    if(strlen(rd_burn_file) == 0) {
	    cJSON_Delete(cjson_tmp);
	    return 0;
    }

    cjson_tmp = cJSON_Parse(rd_burn_file);
    char *device_id;
    device_id = malloc(20);
    if(cJSON_GetObjectItem(cjson_tmp, "device_id") == NULL) return 0;
    device_id = cJSON_GetObjectItem(cjson_tmp, "device_id")->valuestring;
    if(strlen(device_id) == 0) {
	cJSON_Delete(cjson_tmp);
        return 0;
    }

    cJSON_Delete(cjson_tmp);
    return 1;
}


char *read_token_from_hardware()
{
    char *access_token;
    char *file_read = json_file_read(HW_JS_INFO);
    cJSON *file_cjson = cJSON_Parse(file_read);
    access_token = cJSON_GetObjectItem(file_cjson,"device_id")->valuestring;
    return access_token;
}

void parse_rx_data(void *pbuf)
{
    rx_cjson = NULL;
    cJSON *cjson_params = NULL;
    printf("parse_rx_data = %s\n", (unsigned char *)pbuf);
    rx_cjson = cJSON_Parse(pbuf);
    
    json_file_write(MEM_JS_INFO, pbuf);

    cjson_params = cJSON_GetObjectItem(rx_cjson, "params");
    char *activate_state;
    activate_state = cJSON_GetObjectItem(cjson_params,"activate_state")->valuestring;
    cJSON_Delete(rx_cjson);
    return;
}

char *json_file_read(char *filename)
{
    FILE *f;
    long len;

    f = fopen(filename,"w+");
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
//    parse_rx_data(file_read_data);
    return file_read_data;
}

void json_file_write(char *filename, char *message)
{
    FILE *fp = fopen(filename, "w+");
    fwrite(message, strlen(message), 1, fp);
    fclose(fp);
    free(message);
}
