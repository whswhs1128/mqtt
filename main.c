/*************************************************************************
	> File Name: main.c
	> Author: LFJ
	> Mail: 
	> Created Time: 2018年09月05日 星期三 13时48分17秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include "pthread.h"
#include "mqtt.h"
#include "unistd.h"
#include "cJSON.h"

cJSON *root;
char *send_str;

int main(int argc, char *argv[])
{
    pthread_t thread_ID;
    pthread_create(&thread_ID, NULL, &cloud_mqtt_thread, NULL);
    pthread_detach(thread_ID);

	init_software_client();
	
    while(1);
    return 0;
}
