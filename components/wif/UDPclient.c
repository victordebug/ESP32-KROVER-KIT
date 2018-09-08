#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"
#include "camera.h"
#include "Agatt_server.h"
#include "esp_gap_ble_api.h"
#include "Yuart.h"
#include "wiring.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

#include "UDPclient.h"

uint32_t UDPcnt=0; 
int sock_fd;


/* ***************************************************
//  brief:   生成UDP数据帧
//  param:   SN   参见aLock通用协议
//  param:   Ver  
//  param:   safe  
//  return： Frame 帧指针
*******************************************************/
uint8_t* UDP_frame (uint8_t SN,uint8_t Ver,uint8_t safe, 
					uint8_t Reply, uint8_t DataType, uint16_t Data_Len, uint8_t* Data_P )
{
	static uint8_t Frame[40] ;
	uint8_t temp_add=0;
		
	Frame[0] = SN;
	Frame[1] = Ver;
	Frame[2] = safe << 4 | Reply;
	Frame[3] = 3;
	
	for(uint8_t i=0;i<13;i++){
		Frame[4+i] = Device_id[i];
	}

	Frame[17] = DataType;
	Frame[18] = Data_Len >> 8;
	Frame[19] = Data_Len;

	for(uint16_t i=0; i<Data_Len; i++)
	{
		Frame[20+i] = *(Data_P++);
		temp_add = temp_add + Frame[20+i];
	}
	Frame[20+Data_Len] = temp_add;    //校验和
	
	return Frame;
}

uint16_t Data_check(uint8_t * data_p )
{
	uint8_t check_data =0;
	uint16_t Frame_data_len=0;

	Frame_data_len = ((uint16_t)data_p[5])<<8 | (uint16_t)(data_p[6]);

	for(uint16_t i = 0;i < Frame_data_len; i++){
		check_data = check_data + data_p[7+i];
	}

	if(check_data == data_p[7+Frame_data_len])
		return Frame_data_len;
	else
		return (uint16_t) DATA_ERROR;
}

uint8_t* get_data(uint8_t * data_p )
{
	uint8_t * data_ptr = &(data_p[7]);
	return data_ptr;
}

					
uint8_t UDPdata_analys_exe(uint8_t* UDPdata )
{

	uint16_t Data_len =	Data_check(UDPdata);
	
	if(Data_len == DATA_ERROR ){
		ESP_LOGD("UDP","Data_err");
		return DATA_ERROR;
	}
	ESP_LOGD("UDP","Data_len:%d",Data_len);	
	
 	uint8_t *data_p = get_data(UDPdata);

	UARTframe  frame = {
	 	.framedata_type = Type_UDP,
		.framedata_len = Data_len,   //Data_len
		.framedata_p = data_p ,    //data_p
		.framedata_reply = 0,
	};
	if(UDPdata[2] & 0x01){
		frame.framedata_reply = 1;
	}
	
	ESP_LOGD("UDP","p_frame%s",data_p);
	xQueueSend(YuartQueue, &frame, 1);  // 传递数据
	
	return 1;
}



#define PORT 8123
#define Buffer_size 1024



void Init_UDPSocket(void)
{
	ESP_LOGD("socket","Init UDP Socket"); 
	struct sockaddr_in server_addr;	

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {  
        ESP_LOGD("socket","failed to create sock_fd!\n");  
    }  

	struct timeval UDPtimeout = {10,0};                 //设置接受阻塞10S	
	//设置阻塞10s
	int ret=setsockopt(sock_fd,SOL_SOCKET,SO_RCVTIMEO,&UDPtimeout,sizeof(UDPtimeout));  
	if(ret == 0 ) {
		ESP_LOGD("socket","set block sucess");
	}

	memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  
    server_addr.sin_addr.s_addr = inet_addr("118.178.86.53");          //118.178.86.53                     
    server_addr.sin_port = htons(PORT);  

	connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));  

	AUDPQueue = xQueueCreate(10, sizeof(UDPframe));        //创建数据指针接受队列

	ESP_LOGD("socket","Init UDP Socket Success4"); 

}

void AUDP_send(void *pvParameters)
{
	UDPframe udpframe = 
	{
	 	.framedata_type = Type_UDP,
		.framedata_len = 0,   
		.framedata_p = NULL,     
		.framedata_reply = 0,
	};
	uint8_t replay =0;
    for(;;)
	{
		if(xQueueReceive(AUDPQueue, &udpframe, 0))
		{
			ESP_LOGD("socket","data:%s",udpframe.framedata_p);
			
			ESP_LOGD("socket","send%d",UDPcnt);
			if(udpframe.framedata_reply == 1)
			{
				replay = Do_Reply;
			}
			else
			{
				replay = No_Reply;
			}
			uint8_t *send_pack =  UDP_frame(UDPcnt++,0,Data_Security,replay,0,udpframe.framedata_len,(uint8_t *)udpframe.framedata_p ); 
			
			send(sock_fd, (char *)send_pack, 21+udpframe.framedata_len, 0);	
		}
		
    }
	vTaskDelete(NULL);
}


void AUDP_recv(void *pvParameters)
{

	uint8_t* UDP_RecBuffer = (uint8_t*)malloc(Buffer_size); 	//设置UDP接受缓存

	memset(UDP_RecBuffer,0,Buffer_size);
	for(;;)
	{
		int UDP_recv_len = recv(sock_fd,(char *)UDP_RecBuffer,Buffer_size,0);	 //buffer 长度100
		if(UDP_recv_len > 0)
		{
			for(uint8_t i=0;i<strlen((char*)UDP_RecBuffer);i++)
				ESP_LOGD("socket","recv:%x",*(UDP_RecBuffer+i));  //逐个打印接受数据

			//	由调用处清除Buffer
			UDPdata_analys_exe(UDP_RecBuffer);
		}		
	}
	free(UDP_RecBuffer);
	vTaskDelete(NULL);
}


