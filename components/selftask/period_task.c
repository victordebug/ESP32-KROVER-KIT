#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "wiring.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "UDPclient.h"
#include "Yuart.h"
#include "ALble.h"
#include "Yuart.h"
#include "Awifi.h"
#include "JpegRTPDeal.h"
/*
*              周期性执行任务


*/
void period_task(void *pvParameters)
{
	for(;;)
	{
		vTaskDelay(400);             //任务延时4S
		err_t err = JpegSocketConnect();
	
		static uint8_t HeartB[4] = {"U_HB"};
		UDPframe udpframe = 
		{
			.framedata_type = Type_UDP,
			.framedata_len = 4,
			.framedata_p = HeartB,
			.framedata_reply = 0,		
		};
		uint8_t piccnt = 10;
		
		if(err == ESP_OK)
		{
			while(1)
			{
				SendPic();	
			}	
		}

		JpegSocketClose();
		
		if(socket_created == 1 )//发送心跳信号 用于保持和服务器的连接
		{   
			ESP_LOGD("period_task", "send heart");
			BaseType_t queueret = xQueueSend(AUDPQueue,&udpframe, 10);
			if(queueret == errQUEUE_FULL)
			{
				ESP_LOGD("period_task", "queue failed");	 //入队错误
			}
		}

		if(wificonfiggot == 1)
		{
			vTaskDelay(200);
			ESP_LOGD("period_task", "restart");
			esp_restart();                            //得到WiFi配置后重启设�?		}

		}
	}	
	vTaskDelete(NULL);
}



