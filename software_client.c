#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "pthread.h"
#include <unistd.h>
#include "mqtt.h"
#include "cJSON.h"
#include "md5.h"

#define SW_CHECKSUM_ATTR        "sw_checksum"
#define SW_CHECKSUM_ALG_ATTR    "sw_checksum_algorithm"
#define SW_SIZE_ATTR            "sw_size"
#define SW_TITLE_ATTR           "sw_title"
#define SW_VERSION_ATTR         "sw_version"
#define SW_STATE_ATTR           "sw_state"
#define TOPIC_HEAD              "v1/devices/me/attributes/request/"

char *send_str_s;
static int chunk_size;
static int request_id;
static int software_request_id;
cJSON *current_software_info;
cJSON *software_info;

char *software_data;
static int target_software_length;
static int chunk_count;
static int current_chunk;
static int software_received;

void request_software_info() {
	printf("request_software_info enter...\n");
    request_id++;
    cJSON *tx_cjson;
    char *topic;
    tx_cjson = cJSON_CreateObject();
    cJSON_AddItemToObject(tx_cjson, "sharedKeys",cJSON_CreateString("sw_checksum,sw_checksum_algorithm,sw_size,sw_title,sw_version"));
    send_str_s = cJSON_PrintUnformatted(tx_cjson);
    topic = malloc(50);
    strcpy(topic, TOPIC_HEAD);
    char *id_tmp;
    id_tmp = malloc(16);
    sprintf(id_tmp, "%d",request_id);
    strcat(topic, id_tmp);
    mqtt_data_write_with_topic(send_str_s, topic);
    //memset(send_str_s, 0, strlen(send_str_s));
    send_str_s = "";
}

void on_connect() {
	printf("onconnect...\n");
    current_software_info = cJSON_CreateObject();
    cJSON_AddItemToObject(current_software_info, "current_sw_title",cJSON_CreateString("Initial"));
    cJSON_AddItemToObject(current_software_info, "current_sw_version",cJSON_CreateString("v0"));
    send_str_s = cJSON_PrintUnformatted(current_software_info);
    mqtt_data_write(send_str_s);
    //memset(send_str_s, 0, strlen(send_str_s));
    send_str_s = "";
    request_software_info();
}

void get_software() {
	printf("get_software\n");
    char *topic;
    send_str_s = malloc(10);
    send_str_s = "";
    topic = malloc(25);
    strcpy(topic, "v2/sw/request/1/chunk/0");
    //char *id_tmp;
    //id_tmp = malloc(16);
    //sprintf(id_tmp, "%d",software_request_id);
    //strcat(topic, id_tmp);
    //strcat(topic, "/chunk/");
    //sprintf(id_tmp, "%d",current_chunk);
    //strcat(topic, id_tmp);
    mqtt_data_write_with_topic(send_str_s, topic);
    //memset(send_str_s, 0, strlen(send_str_s));
    send_str_s = "";
}

void on_message_data(void *pbuf) {
	printf("on_messgae_data...\n");
    cJSON *rx_cjson;
    rx_cjson = cJSON_Parse(pbuf);
    software_info = cJSON_GetObjectItem(rx_cjson, "shared");
    if(!strcmp(cJSON_GetObjectItem(software_info,"sw_title")->valuestring, cJSON_GetObjectItem(current_software_info,"current_sw_title")->valuestring)==0 ||
    !strcmp(cJSON_GetObjectItem(software_info,"sw_version")->valuestring, cJSON_GetObjectItem(current_software_info,"current_sw_version")->valuestring)==0
    ) {
        printf("Software is not the same\n");
    	software_data = "";
        current_chunk = 0;
        cJSON_AddItemToObject(current_software_info, SW_STATE_ATTR,cJSON_CreateString("DOWNLOADING"));
        send_str_s = cJSON_PrintUnformatted(current_software_info);
        mqtt_data_write(send_str_s);
        sleep(1);
        //memset(send_str_s, 0, strlen(send_str_s));
    	send_str_s = "";

        software_request_id++;
        target_software_length = cJSON_GetObjectItem(software_info,"sw_size")->valueint;
        software_data = malloc(target_software_length);
        get_software();
    }
	printf("on_messgae_data end...\n");
}

int verify_checksum(char *software_data, char *checksum_alg, char *checksum) {
    char *md5_str;
    md5_str = malloc(32);
    if (strlen(software_data) == 0) 
    {
        printf("Software wasn't received!\n");
        return 0;
    }

    if (strlen(checksum) == 0)
    {   
        printf("checksum wasn't provided!\n");
        return 0;
    }

    if (!strcmp(checksum_alg, "MD5") == 0)
    {
        printf("checksum type wasn't supported!\n");
        return 0;
    }
    
    
    Compute_data_md5(software_data, strlen(software_data), md5_str);
    printf("compute md5 is %s\n",md5_str);
    if(strcmp(checksum, md5_str) == 0)return 1;

}

void process_software() {
    cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("DOWNLOADED"));
    send_str_s = cJSON_PrintUnformatted(current_software_info);
	printf("process_software:send_str_s is %s\n",send_str_s);
    mqtt_data_write(send_str_s);
    sleep(1);
    //memset(send_str_s, 0, strlen(send_str_s));
    send_str_s = "";
    
    int verification_result;
    verification_result = verify_checksum(software_data,cJSON_GetObjectItem(software_info,SW_CHECKSUM_ALG_ATTR)->valuestring, 
                                                        cJSON_GetObjectItem(software_info,SW_CHECKSUM_ATTR)->valuestring);

    if(verification_result) {
        printf("Checksum verified!\n");
        cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("VERIFIED"));
        send_str_s = cJSON_PrintUnformatted(current_software_info);
        mqtt_data_write(send_str_s);
        sleep(1);
        //memset(send_str_s, 0, strlen(send_str_s));
    	send_str_s = "";
        software_received = 1;
    } else {
        printf("Checksum verification failed!\n");
        cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("FAILED"));
        send_str_s = cJSON_PrintUnformatted(current_software_info);
        mqtt_data_write(send_str_s);
        sleep(1);
        //memset(send_str_s, 0, strlen(send_str_s));
    	send_str_s = "";
        request_software_info();
    }
}

void on_message_bin(void *pbuf) {
	printf("on_messgae_bin...\n");
    char *software_data_tmp;
    software_data_tmp = malloc(strlen(pbuf));
    strcpy(software_data_tmp, pbuf);
    strcpy(software_data, software_data_tmp);
    current_chunk++;
    if (strlen(software_data) == target_software_length)
    {
        process_software();
    } else {
        get_software();
    }
    

}

void *update_thread() {
	printf("update_thread enter...\n");
    while (1) {
        if (software_received == 1)
        {
	    printf("software_received...\n");
            cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("UPDATING"));
            send_str_s = cJSON_PrintUnformatted(current_software_info);
            mqtt_data_write(send_str_s);
            sleep(1);
            //memset(send_str_s, 0, strlen(send_str_s));
    	    send_str_s = "";

            char *filename;
            filename = cJSON_GetObjectItem(software_info,SW_TITLE_ATTR)->valuestring;
            FILE *fp = fopen(filename, "wb");
            fwrite(software_data, strlen(software_data), 1, fp);
            fclose(fp);
	    printf("saved file sucess...\n");

            cJSON_Delete(current_software_info);
            current_software_info = cJSON_CreateObject();
            cJSON_AddItemToObject(current_software_info, "current_sw_title",cJSON_GetObjectItem(software_info,SW_TITLE_ATTR));
            cJSON_AddItemToObject(current_software_info, "current_sw_version",cJSON_GetObjectItem(software_info,SW_VERSION_ATTR));
            send_str_s = cJSON_PrintUnformatted(current_software_info);
            mqtt_data_write(send_str_s);
            //memset(send_str_s, 0, strlen(send_str_s));
    	    send_str_s = "";
            software_received = 0;
            sleep(1);
        }
       sleep(1);
//      printf("in update_thread...\n"); 
    }
}

void init_software_client() {

     on_connect();
//	get_software();
    pthread_t thread_ID;
    pthread_create(&thread_ID, NULL, &update_thread, NULL);
    pthread_detach(thread_ID);

}
