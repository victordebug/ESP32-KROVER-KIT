#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

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
#include "nvs.h"
#include "driver/gpio.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"


#include "UDPclient.h"
#include "Awifi.h"
#include "JpegRTPDeal.h"

#define version 2
#define padding 0
#define extend  0
#define csrc_count 0
#define JPEGHeaderLEN 256

unsigned short serial_number;
int32_t time_stamp = 100000;


int32_t TimeStamp()
{
	time_stamp += 20000;

	return time_stamp;
}

/*
	brife: RTP数据包头
	param：mark :   标识一段序列的结束
		   pt   ：  记录使用哪种载荷 编解码
		   seq  ： 序列号 每个数据包加1

	return : 返回数据头指针
*
*/
uint8_t * RTPHead(uint32_t ts, uint8_t mark, uint8_t pt, uint16_t seq )
{
	static uint8_t  RTPheader[12] = {0};
	int32_t Time_stamp = ts;
	
	RTPheader[0] = (version << 6 | padding << 5 | extend << 4 | csrc_count);
	RTPheader[1] = ( mark << 7 | pt );
	RTPheader[2] = (uint8_t)(seq >> 8);
	RTPheader[3] = (uint8_t)seq;
	RTPheader[4] = (uint8_t)(Time_stamp >> 24);
	RTPheader[5] = (uint8_t)(Time_stamp >> 16);
	RTPheader[6] = (uint8_t)(Time_stamp >> 8);
	RTPheader[7] = (uint8_t)Time_stamp;

	return RTPheader;
}


uint8_t * JpegHead(uint32_t piclen )
{
	static uint8_t Jpegheader[JPEGHeaderLEN] = {0};
	uint16_t   usSturctlen = JPEGHeaderLEN;
	uint8_t    ucSnapCount = 1;
	uint8_t    ucSnapIndex = 0;
	
	Jpegheader[0] = (uint8_t)(usSturctlen >> 8);      //数据头的长度  保证兼容性
	Jpegheader[1] = (uint8_t)usSturctlen;

	Jpegheader[4] = (uint8_t)(piclen >> 24);          //图片长度
	Jpegheader[5] = (uint8_t)(piclen >> 16);
	Jpegheader[6] = (uint8_t)(piclen >> 8);
	Jpegheader[7] = (uint8_t)piclen;          
      
	Jpegheader[10] = (uint8_t)ucSnapCount;          //图片数量
	Jpegheader[11] = (uint8_t)ucSnapIndex;          

	return Jpegheader;
}


#define JPEGPORT 13500
static struct sockaddr_in JpegServer_addr;	
static int Jpegsock_fd; 

void InitJpegSocket()
{
/*	
	Jpegsock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(Jpegsock_fd == -1) {  
        ESP_LOGD("socket","failed to create sock_fd!\n");  
    }  

	memset(&JpegServer_addr, 0, sizeof(JpegServer_addr));  
    JpegServer_addr.sin_family = AF_INET;  
    JpegServer_addr.sin_addr.s_addr = inet_addr("192.168.2.140");          //118.178.86.53                     
    JpegServer_addr.sin_port = htons(JPEGPORT);  

	//connect(Jpegsock_fd, (struct sockaddr *)&JpegServer_addr, sizeof(struct sockaddr));  

	ESP_LOGD("socket","Init JPEG Socket Success"); 
*/	
}

err_t JpegSocketConnect()
{
	Jpegsock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(Jpegsock_fd == -1) 
	{  
        ESP_LOGD("socket","failed to create sock_fd!\n");  
    }  

	memset(&JpegServer_addr, 0, sizeof(JpegServer_addr));  
    JpegServer_addr.sin_family = AF_INET;  
    JpegServer_addr.sin_addr.s_addr = inet_addr("192.168.10.102");          //118.178.86.53                     
    JpegServer_addr.sin_port = htons(JPEGPORT);  

	err_t err = connect(Jpegsock_fd, (struct sockaddr *)&JpegServer_addr, sizeof(struct sockaddr));  
	if(err != ESP_OK)
	{  
		ESP_LOGE("socket", "socket connect err: %d",err);
		return err;
	}
	return ESP_OK;
}

err_t JpegSocketClose()
{
	err_t err = closesocket(Jpegsock_fd);
	if(err != ESP_OK){
		ESP_LOGE("socket", "socket close err: %d",err);
		return err;
	}
	return ESP_OK;
}

uint16_t PicSeq = 1; 

void SendPic ()
{
	uint32_t TS = 0;
	uint8_t pdu1[4] = { 0X24, 0X00, 0X00, 0X00 };
	uint8_t pdu2[4] = { 0X24, 0X00, 0X05, 0X84 };
 	uint8_t ID[13] = {Device_id};
	
	err_t err = camera_run();
	size_t PicLen = camera_get_data_size();
	size_t PicLenCnt = PicLen;
	ESP_LOGD("JpegPIC", "data len %d",PicLen);
	uint8_t *fb_p = camera_get_fb();

	if(err == ERR_OK)
	{
		TS = TimeStamp();	     //一张图片时间戳相同
/*
		send(Jpegsock_fd,pdu2,4,0);   //发送第一个数据包         包含Deviceid
		send(Jpegsock_fd,RTPHead(TS,0,0,PicSeq++),12,0);
		send(Jpegsock_fd,Device_id,13,0);
		send(Jpegsock_fd,fb_p,1387 ,0);	
		PicLenCnt -= 1387;
		fb_p += 1387;
*/
		while(PicLenCnt > 1400)
		{
			send(Jpegsock_fd,pdu2,4,0);
			send(Jpegsock_fd,RTPHead(TS,0,0
				,PicSeq++),12,0);
			send(Jpegsock_fd,fb_p,1400 ,0);	
			PicLenCnt -= 1400;
			fb_p += 1400;
		}

		pdu1[2] = (((PicLen+13) % 1400) +12) / 256;
		pdu1[3] = (((PicLen+13) % 1400) +12) % 256;

		send(Jpegsock_fd,pdu1,4,0);
		send(Jpegsock_fd,RTPHead(TS,1,0,PicSeq++),12,0);   //最后一个数据包设置mark为1
		send(Jpegsock_fd,fb_p,PicLenCnt ,0);


		send(Jpegsock_fd,ID,13,0);  //发送ID

	}
	else
	{
		ESP_LOGE("camera","camera error");
	}

	ESP_LOGD("JpegPIC","send Pic OK");

}


void PicSend(void *pvParameters)
{
	PicEventGroup = xEventGroupCreate();
	if(PicEventGroup == NULL)
	{
		ESP_LOGE("JpegPIC","Creat PicEventGroup failed");
	}
	EventBits_t uxBits;

	for(;;)
	{	
		uxBits = xEventGroupWaitBits(
					PicEventGroup,		// 
					PicSendEevent,	   	 	// 
					pdTRUE,				// 在返回时设置位被清除
					pdFALSE,			// 不用所有位都被置位就会返回
					portMAX_DELAY );	// Wait a maximum
					
		if((uxBits & PicSendEevent) == PicSendEevent)
		{
			while(1)
			{
				SendPic();
			}
		}
    }
	vEventGroupDelete(PicEventGroup);
	vTaskDelete(NULL);
	
}



