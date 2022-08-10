#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pthread.h"

#define HOST_IP "192.168.1.104"
const int PORT = 1777;
int device_id_flag;
int date_flag;
int flow_id_flag;

int sockfd;
struct sockaddr_in serverAddr;
char *buf;


void udp_send_thread() {
    while(!(device_id_flag && date_flag && flow_id_flag)) {
        if(!device_id_flag) {
            memset(buf,0,strlen(buf));
            strcpy(buf,"ask id");
            sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
        } else {
            if(!date_flag) {
            memset(buf,0,strlen(buf));
            strcpy(buf,"ask date");
            sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
            }

            if(!flow_id_flag) {
            memset(buf,0,strlen(buf));
            strcpy(buf,"ask flow id");
            sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
            }
        }
        sleep(10);
    }
    pthread_exit(0);
}

char *device_id;
char *burn_date;
char *flow_id;
void udp_recv_thread() {

    while (!(device_id_flag && date_flag && flow_id_flag))
    {
        memset(buf,0,strlen(buf));
        recvfrom(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
        int length = strlen(buf);
        if(length == 8) {
            strcpy(burn_date, buf);
            date_flag = 1;
        }

        if(length > 8) {
            strcpy(device_id, buf);
            device_id_flag = 1;
        }

        if(length < 8) {
            strcpy(flow_id, buf);
            flow_id_flag = 1;
        }
    }

    memset(buf,0,strlen(buf));
    strcpy(buf,"recv ok");
    sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    format_hardware_str(device_id, burn_date, flow_id);
    close(sockfd);
    pthread_exit(0);
}

void start_udp_client() {
  char buffer[1024];
  socklen_t addr_size;

  sockfd = socket(PF_INET, SOCK_DGRAM, 0);
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

    while(1) {
        if(device_id_flag && date_flag && flow_id_flag) {
            printf("success..\n");
            return;
        }
    }


}