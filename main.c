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
    while(!get_burning_info_from_file()) {
	    printf("before start..\n");
	    start_udp_client();
	    sleep(5);
    }

    pthread_create(&thread_ID, NULL, &cloud_mqtt_thread, NULL);
    pthread_detach(thread_ID);

    system("/opt/custom/mqtt_ota_demo &");
    while (1) {
	    if(!get_activate_state()) {
		    send_str = format_first_init();
	    } else {
		    send_str = format_heartbeat_str();
	    }
	    mqtt_data_write(send_str, strlen(send_str), 0);
	    memset(send_str,0,strlen(send_str));
	    sleep(65);
    }

    return 0;
}
