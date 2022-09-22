/*************************************************************************
	> File Name: matt.h
	> Author: 
	> Mail: 
	> Created Time: 2018???09???05?¡ª? ????????? 16?¡ª?40???31?¡ì?
 ************************************************************************/

#ifndef NET_PROC_H
#define NET_PROC_H

#ifdef __cplusplus
extern "C" {
#endif

enum  iot_ctrl_status_t
{
	IOT_STATUS_LOGIN,
	IOT_STATUS_CONNECT,
	IOT_STATUS_DROP,
};

typedef void  (*pMessageArrived_Fun)(void*,int len);

void mqtt_module_init(void);
void init_software_client();
int mqtt_data_write(char *pbuf);
int mqtt_data_write_with_topic(char *pbuf, char *topic);
char *format_send_str();
void json_file_write();
char *json_file_read();
int get_burning_info_from_file();
void format_hardware_str(char *device_id, char *burn_date, char *flow_id);
void start_udp_client();
int get_activate_state();
char *format_first_init();
char *format_heartbeat_str();

void mqtt_data_rx_cb(void *pbuf, int len);
void mqtt_data_rx_bin(void *pbuf, int len);
void mqtt_data_rx_data(void *pbuf, int len);
void on_message_bin();
void on_message_data(void *pbuf);
void parse_rx_data(void *pbuf);
void *cloud_mqtt_thread(void *arg);
#define mDEBUG(fmt, ...)  printf("%s[%s](%d):" fmt,__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
