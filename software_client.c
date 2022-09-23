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
#define CURRENT_VERSION		"V1.0.0.0"
char *send_str_s;
static int request_id;
static int software_request_id;
static cJSON *current_software_info;
static cJSON *software_info;

static int current_chunk;
static int software_received;

void request_software_info() {
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
    send_str_s = "";
}

void on_connect() {
	if(!cJSON_GetArraySize(current_software_info))
    {
		current_software_info = cJSON_CreateObject();
		cJSON_AddItemToObject(current_software_info, "current_sw_title",cJSON_CreateString("ipc_ota"));
		cJSON_AddItemToObject(current_software_info, "current_sw_version",cJSON_CreateString(CURRENT_VERSION));
	}
	cJSON_DeleteItemFromObject(current_software_info, SW_STATE_ATTR);
	send_str_s = cJSON_PrintUnformatted(current_software_info);
	mqtt_data_write(send_str_s);
	send_str_s = "";
	request_software_info();
}

void get_software() {
    char *topic;
    send_str_s = malloc(10);
    send_str_s = "";
    topic = malloc(25);
    //strcpy(topic, "v2/sw/request/1/chunk/0");
    strcpy(topic, "v2/sw/request/");
    char *id_tmp;
    id_tmp = malloc(16);
    sprintf(id_tmp, "%d",software_request_id);
    strcat(topic, id_tmp);
    strcat(topic, "/chunk/");
    sprintf(id_tmp, "%d",current_chunk);
    strcat(topic, id_tmp);
    mqtt_data_write_with_topic(send_str_s, topic);
    send_str_s = "";
}

void on_message_data(void *pbuf) {
    cJSON *rx_cjson;
    rx_cjson = cJSON_Parse(pbuf);
    software_info = cJSON_GetObjectItem(rx_cjson, "shared");
    if(!strcmp(cJSON_GetObjectItem(software_info,"sw_title")->valuestring, cJSON_GetObjectItem(current_software_info,"current_sw_title")->valuestring)==0 ||
    !strcmp(cJSON_GetObjectItem(software_info,"sw_version")->valuestring, cJSON_GetObjectItem(current_software_info,"current_sw_version")->valuestring)==0
    ) {
        printf("Software is not the same\n");
        current_chunk = 0;
        cJSON_AddItemToObject(current_software_info, SW_STATE_ATTR,cJSON_CreateString("DOWNLOADING"));
        send_str_s = cJSON_PrintUnformatted(current_software_info);
        mqtt_data_write(send_str_s);
        sleep(1);
    	send_str_s = "";

        software_request_id++;
        get_software();
    }
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
    
    
    Compute_file_md5(, md5_str);
    //printf("compute md5 is %s\n",md5_str);
    if(strcmp(checksum, md5_str) == 0)return 1;

    return 0;

}

long get_file_size(char *filename)
{
	long n;
	FILE *fp1 = fopen(filename,"r");
	fpos_t fpos; //当前位置
	fgetpos(fp1, &fpos); //获取当前位置
	fseek(fp1, 0, SEEK_END);
	n = ftell(fp1);
	fsetpos(fp1,&fpos); //恢复之前的位置
	return n;
}


void process_software() {
    cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("DOWNLOADED"));
    send_str_s = cJSON_PrintUnformatted(current_software_info);
    mqtt_data_write(send_str_s);
    sleep(1);
    send_str_s = "";
   
//    long file_size;
//    file_size = get_file_size("tmpfile");
//    printf("filesize is %ld\n",file_size);
    int verification_result;
    verification_result = verify_checksum("tmpfile",cJSON_GetObjectItem(software_info,SW_CHECKSUM_ALG_ATTR)->valuestring,
                                                        cJSON_GetObjectItem(software_info,SW_CHECKSUM_ATTR)->valuestring);

    if(verification_result) {
    //if(file_size == cJSON_GetObjectItem(software_info,SW_SIZE_ATTR)->valueint) {
        printf("Checksum verified!\n");
        cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("VERIFIED"));
        send_str_s = cJSON_PrintUnformatted(current_software_info);
        mqtt_data_write(send_str_s);
        sleep(1);
    	send_str_s = "";
        software_received = 1;
    } else {
        printf("Checksum verification failed!\n");
        cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("FAILED"));
        send_str_s = cJSON_PrintUnformatted(current_software_info);
        mqtt_data_write(send_str_s);
        sleep(1);
    	send_str_s = "";
	    system("rm tmpfile");
        //request_software_info();
    }
}

void on_message_bin() {
    process_software();
}

int copy_by_block(const char *dest_file_name, const char *src_file_name) {//ok
    FILE *fp1 = fopen(dest_file_name,"w");
    FILE *fp2 = fopen(src_file_name,"r");
    if(fp1 == NULL) {
        perror("fp1:");
        return -1;
    }
    if(fp2 == NULL) {
        perror("fp2:");
        return -1;
    }
    void *buffer = (void *)malloc(2);
    int cnt = 0;
    while(1) {
        int op = fread(buffer,1,1,fp2);
        if(op <= 0) break;
        fwrite(buffer,1,1,fp1);
        cnt++;
    }
    free(buffer);
    fclose(fp1);
    fclose(fp2);
    return cnt;
}


void *update_thread() {
    while (1) {
        if (software_received == 1)
        {
            cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR, cJSON_CreateString("UPDATING"));
            send_str_s = cJSON_PrintUnformatted(current_software_info);
            mqtt_data_write(send_str_s);
            sleep(1);
    	    send_str_s = "";

            char *filename;
            filename = cJSON_GetObjectItem(software_info,SW_TITLE_ATTR)->valuestring;
	        copy_by_block(filename, "tmpfile");
	        char *command;
	        command = malloc(30);
	        strcpy(command, "chmod 777 ");
	        strcat(command,filename);
	        system(command);
	        system("rm tmpfile");
	        printf("saved file sucess...\n");

//            cJSON_Delete(current_software_info);
//            current_software_info = cJSON_CreateObject();
	        cJSON_ReplaceItemInObject(current_software_info, "current_sw_title",cJSON_CreateString(cJSON_GetObjectItem(software_info,SW_TITLE_ATTR)->valuestring));
	        cJSON_ReplaceItemInObject(current_software_info, "current_sw_version",cJSON_CreateString(cJSON_GetObjectItem(software_info,SW_VERSION_ATTR)->valuestring));
            cJSON_ReplaceItemInObject(current_software_info, SW_STATE_ATTR,cJSON_CreateString("UPDATED"));
            send_str_s = cJSON_PrintUnformatted(current_software_info);
            mqtt_data_write(send_str_s);
    	    send_str_s = "";
            software_received = 0;
            sleep(1);
        }
       sleep(2);
    }
}

void init_software_client() {

//     on_connect();
    pthread_t thread_ID;
    pthread_create(&thread_ID, NULL, &update_thread, NULL);
    pthread_detach(thread_ID);

    while(1) {
	    on_connect();
	    sleep(60);
    }
}
