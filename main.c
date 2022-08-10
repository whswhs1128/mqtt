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
int get_burning_id_from_file();

int main(int argc, char *argv[])
{
    pthread_t thread_ID;		//定义线程id
	if(!get_burning_info_from_file()) {
		start_udp_client();
	}

    pthread_create(&thread_ID, NULL, &cloud_mqtt_thread, NULL);	//创建一个线程执行mqtt客户端
    pthread_detach(thread_ID);	//设置线程结束收尸

    char *pbuf;
    while (1) {
	send_str = format_send_str();
	mqtt_data_write(send_str, strlen(send_str), 0);
	//pbuf = json_file_read(filename);
	//json_file_write("new.json",pbuf);
	sleep(3);						//睡眠3s
    }

    return 0;
}
