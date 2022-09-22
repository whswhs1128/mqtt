/*************************************************************************
	> File Name: main.c
	> Author: LFJ
	> Mail: 
	> Created Time: 2018年09月05日 星期三 13时48分17秒
 ************************************************************************/

#include <stdio.h>

#include "MQTTClient.h"
#include "mqtt.h"
#include "pthread.h"
#include "string.h"
#include "unistd.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "fcntl.h"


#define MQTT_TOPIC_SIZE     (128)		//订阅和发布主题长度
#define MQTT_BUF_SIZE       (8 * 1024) 	//接收后发送缓冲区大小

//#define MQTT_HOST "192.168.1.20"		//ip地址
#define MQTT_HOST "10.41.10.37"		//ip地址
#define MQTT_PORT 1883					//端口号
//#define MQTT_USER "JDwski43V8tuNkZHVn2L"				//用户名
#define MQTT_USER "0eQo1wxhO9YEKMQf83X4"				//用户名
#define MQTT_PASS "cec123"			//密码
#define MQTT_CLIENT_ID "17849359"		//客户端标识
typedef unsigned char   byte;

char *read_token_from_hardware();
typedef struct {
    Network Network;
    Client Client;
    char sub_topic[MQTT_TOPIC_SIZE];		//存放订阅主题
    char pub_topic[MQTT_TOPIC_SIZE];		//存放发布主题
    char mqtt_buffer[MQTT_BUF_SIZE];		//发送缓冲区
    char mqtt_read_buffer[MQTT_BUF_SIZE];	//接收缓冲区

    unsigned char willFlag;					
    MQTTPacket_willOptions will;
    char will_topic[MQTT_TOPIC_SIZE];		//存放遗嘱主题

    pMessageArrived_Fun DataArrived_Cb;
}Cloud_MQTT_t;

typedef struct{
    enum iot_ctrl_status_t iotstatus;
    char model[5];
    char company[32];
} iot_device_info_t;//主题结构体


struct opts_struct {
    char    *clientid;
    int     nodelimiter;
    char    *delimiter;
    enum    QoS qos;
    char    *username;
    char    *password;
    char    *host;
    int     port;
    int     showtopics;
} opts = {
    (char *)"iot-dev", 0, (char *)"\n", QOS0, "admin", "password", (char *)"localhost", 1883, 0
};//初始化结构体

Cloud_MQTT_t Iot_mqtt;

iot_device_info_t gateway = {
    .iotstatus = IOT_STATUS_LOGIN,
    .model = {"2017"},
    .company = {"/my"}
};//初始化主题


void iot_mqtt_init(Cloud_MQTT_t *piot_mqtt) 
{
    memset(piot_mqtt, '\0', sizeof(Cloud_MQTT_t));

    sprintf(piot_mqtt->sub_topic, "v1/devices/me/attributes/response/+");	//将初始化好的订阅主题填到数组中
    printf("subscribe:%s\n", piot_mqtt->sub_topic);

    sprintf(piot_mqtt->pub_topic, "v1/devices/me/telemetry");	//将初始化好的发布主题填到数组中
    printf("pub:%s\n", piot_mqtt->pub_topic);

    piot_mqtt->DataArrived_Cb = mqtt_data_rx_cb;		//设置接收到数据回调函数

}
void MQTTMessageArrived_Cb(MessageData* md)
{
    MQTTMessage *message = md->message; 

    Cloud_MQTT_t *piot_mqtt = &Iot_mqtt;

    if (NULL != piot_mqtt->DataArrived_Cb) {
        piot_mqtt->DataArrived_Cb((void *)message->payload, message->payloadlen);//异步消息体
    }
}
// 第二主题回调
void MQTTMessageArrived_Cb_new(MessageData* md)
{
    MQTTMessage *message = md->message; 

    Cloud_MQTT_t *piot_mqtt = &Iot_mqtt;

    printf("cb_new......\n");
    mqtt_data_rx_bin((void *)message->payload, message->payloadlen);//异步消息体
}

int mqtt_device_connect(Cloud_MQTT_t *piot_mqtt)
{
    int rc = 0, ret = 0;

    NewNetwork(&piot_mqtt->Network);

    //printf("topic = %s\n", piot_mqtt->sub_topic);

    rc = ConnectNetwork(&piot_mqtt->Network, MQTT_HOST, (int)MQTT_PORT);	
    if (rc != 0) {
        printf("mqtt connect network fail \n");
        ret = -101;
        goto __END;
    }
    MQTTClient(&piot_mqtt->Client, &piot_mqtt->Network, 1000, piot_mqtt->mqtt_buffer, MQTT_BUF_SIZE, piot_mqtt->mqtt_read_buffer, MQTT_BUF_SIZE);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    if (piot_mqtt->willFlag) {
        data.willFlag = 1;
        memcpy(&data.will, &piot_mqtt->will, sizeof(MQTTPacket_willOptions));
    } else {
        data.willFlag = 0;
    }
    data.MQTTVersion = 3;
    data.clientID.cstring = MQTT_CLIENT_ID;
    data.username.cstring = MQTT_USER;
//    data.username.cstring = read_token_from_hardware();
    data.password.cstring = MQTT_PASS;
    data.keepAliveInterval = 30;
    data.cleansession = 0;
    
    rc = MQTTConnect(&piot_mqtt->Client, &data);
    if (rc) {
        printf("mqtt connect broker fail \n");
        printf("rc = %d\n", rc);
        ret = -102;
        goto __END;
    }
    rc = MQTTSubscribe(&piot_mqtt->Client, piot_mqtt->sub_topic, 0, MQTTMessageArrived_Cb);
    if (rc) {
        printf("mqtt subscribe fail \n");
        ret = -105;
        goto __END;
    }

// 第二主题订阅
    rc = MQTTSubscribe(&piot_mqtt->Client, "v2/sw/response/+/chunk/+", 1, MQTTMessageArrived_Cb_new);
    if (rc) {
        printf("mqtt subscribe second topic fail \n");
        ret = -105;
        goto __END;
    }
    //on_connect();
    gateway.iotstatus = IOT_STATUS_CONNECT;
    printf("Subscribed %d\n", rc);

__END:
    return ret;
}

int mqtt_device_disconnect(Cloud_MQTT_t *piot_mqtt)//断开mqtt连接
{
    int ret = 0;

    ret = MQTTDisconnect(&piot_mqtt->Client);
    printf("disconnectNetwork ret = %d\n", ret);

    return ret;
}

void iot_yield(Cloud_MQTT_t *piot_mqtt)
{
    int ret;

    switch (gateway.iotstatus) {
        case IOT_STATUS_LOGIN:
        ret = mqtt_device_connect(piot_mqtt);
        if (ret < 0) {
            printf("iot connect error code %d\n", ret);
            sleep(1);
        }
        break;
        case IOT_STATUS_CONNECT:
        ret = MQTTYield(&piot_mqtt->Client, 100);
        if (ret != SUCCESS) {
            gateway.iotstatus = IOT_STATUS_DROP;
        }
        break;
        case IOT_STATUS_DROP:
        printf("IOT_STATUS_DROP\n");
        mqtt_device_disconnect(piot_mqtt);
        gateway.iotstatus = IOT_STATUS_LOGIN;
        usleep(1000);
        break;
        default:
        break;
    }
}

int mqtt_will_msg_set(Cloud_MQTT_t *piot_mqtt, char *pbuf, int len)//设置遗嘱函数
{
    memset(piot_mqtt->will_topic, '\0', MQTT_TOPIC_SIZE);
    MQTTPacket_willOptions mqtt_will = MQTTPacket_willOptions_initializer;

    strcpy(piot_mqtt->will_topic, Iot_mqtt.pub_topic);
    memcpy(&Iot_mqtt.will, &mqtt_will, sizeof(MQTTPacket_willOptions));

    Iot_mqtt.willFlag = 1;
    Iot_mqtt.will.retained = 1;
    Iot_mqtt.will.topicName.cstring = (char *)piot_mqtt->will_topic;
    Iot_mqtt.will.message.cstring = (char *)pbuf;
    Iot_mqtt.will.qos = QOS1;

}

void mqtt_data_rx_cb(void *pbuf, int len) 
{
    printf("rx_cb is %s\n",pbuf);
	on_message_data(pbuf);
}

// 接收二进制回调函数
void mqtt_data_rx_bin(void *pbuf, int len) 
{
	unsigned char tmp[len];
	memcpy(tmp, pbuf, len);

	char *test;
	test = malloc(len * 2);
	char *tmp_str;
	tmp_str = malloc(4);
	int i;
	FILE * fp;
	fp = fopen ("tmpfile", "w+");
	for(i =0; i< len; i++) {
//		sprintf(tmp_str, "%c",tmp[i]);
//		strcat(test,tmp_str);
		fprintf(fp,"%c",tmp[i]);
	}
	fclose(fp);
	//system("chmod 777 tmpfile");
//	printf("test is %s\n",test);

	on_message_bin(test);
}

int mqtt_data_write(char *pbuf)
{
    Cloud_MQTT_t *piot_mqtt = &Iot_mqtt; 
    int ret = 0;
    MQTTMessage message;
    char my_topic[128] = {0};

    strcpy(my_topic, piot_mqtt->pub_topic);

    printf("publish message is %s\n",pbuf);
    printf("publish topic is :%s\r\n", my_topic);

    message.payload = (void *)pbuf;
    message.payloadlen = strlen(pbuf);
    message.dup = 0;
    message.qos = QOS0;
    message.retained = 0;


    ret = MQTTPublish(&piot_mqtt->Client, my_topic, &message);	//发布一个主题

    return ret;
}

int mqtt_data_write_with_topic(char *pbuf, char *topic)
{
    Cloud_MQTT_t *piot_mqtt = &Iot_mqtt; 
    int ret = 0;
    MQTTMessage message;
    //char my_topic[128] = {0};

    //strcpy(my_topic, piot_mqtt->pub_topic);

    printf("publish message is %s\n",pbuf);
    printf("publish topic is :%s\r\n", topic);

    message.payload = (void *)pbuf;
    message.payloadlen = strlen(pbuf);
    message.dup = 0;
    message.qos = QOS0;
    message.retained = 0;

    ret = MQTTPublish(&piot_mqtt->Client, topic, &message);	//发布一个主题

    return ret;
}

void *cloud_mqtt_thread(void *arg)
{
    int ret, len; 
    char will_msg[256] = {"hello world"};						//初始化遗嘱数据
    
    iot_mqtt_init(&Iot_mqtt);									//初始化主题
//    mqtt_will_msg_set(&Iot_mqtt, will_msg, strlen(will_msg));	//设置遗嘱

    ret = mqtt_device_connect(&Iot_mqtt);						//初始化并连接mqtt服务器
    while (ret < 0) {
        printf("ret = %d\r\n", ret);
        sleep(3);
        ret = mqtt_device_connect(&Iot_mqtt);
    }

    while (1){
        iot_yield(&Iot_mqtt);									//维持服务器稳定，断开重连
    }
}
