#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pthread.h"
#include <unistd.h>

#define HOST_IP "192.168.16.104"
const int PORT = 1777;
int device_id_flag;
int date_flag;
int flow_id_flag;

int sockfd;
struct sockaddr_in serverAddr;
char *buf;
void format_hardware_str();

void *udp_send_thread() {
    while(!(device_id_flag && date_flag && flow_id_flag)) {
        if(!device_id_flag) {
	    buf = "ask1";
            sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
        } else {
            if(!date_flag) {
            buf = "ask2";
            sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
            }

            if(!flow_id_flag) {
	    buf = "ask3";
            sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
            }
        }
        sleep(3);
    }
    pthread_exit(0);
}

char *device_id;
char *burn_date;
char *flow_id;
void *udp_recv_thread() {

	char recv_buf[100];
	
	while(!(device_id_flag && date_flag && flow_id_flag)) {
		socklen_t len = sizeof(serverAddr);
		memset(recv_buf,0,100);
		recvfrom(sockfd,recv_buf,sizeof(recv_buf),0,(struct sockaddr*)&serverAddr,&len);
		int length = strlen(recv_buf);
		printf("length is %d\n",length);
		printf("buf is %s\n",recv_buf);
		if(length == 8) {
			burn_date =(char*)malloc(length+1);
			strcpy(burn_date, recv_buf);
			printf("date is %s\n",burn_date);
			date_flag = 1;
		}

		if(length == 20) {
			device_id =(char*)malloc(length+1);
			strcpy(device_id, recv_buf);
			printf("deviceid is %s\n",device_id);
			device_id_flag = 1;
		}

		if(length < 4) {
			flow_id =(char*)malloc(length+1);
			strcpy(flow_id, recv_buf);
			printf("flowid is %s\n",flow_id);
			flow_id_flag = 1;
		}

	}
	buf = "recv";
	sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
	printf("device_id is %s,burn_date is %s, flow_id is %s",device_id,burn_date,flow_id);
	format_hardware_str(device_id, burn_date, flow_id);
	pthread_exit(0);
	close(sockfd);
	return NULL;
}

void start_udp_client() {
    char buffer[1024];
    socklen_t addr_size;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    printf("start connect udp server...\n");
    memset(&serverAddr, '\0', sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(HOST_IP);

    pthread_t send_thread = 0;
    pthread_t recv_thread = 1;

    pthread_create(&send_thread, NULL, udp_send_thread, NULL);	
    pthread_detach(send_thread);

    pthread_create(&recv_thread, NULL, udp_recv_thread, NULL);	
    pthread_detach(recv_thread);
}
