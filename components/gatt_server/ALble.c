#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bt.h"
#include "bta_api.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"

#include "sdkconfig.h"

#include "ALble.h"
#include "Yuart.h"
#include "Agatt_server.h"
#include "Awifi.h"
#include "esp_log.h"


uint8_t BleSN = 0;
uint8_t gatt_a_if=0;
uint8_t BLEACK = 0;
uint8_t wificonfiggot = 0;


Awifi_config Ble_Get_W_config;


/* **************************************************
//  brief:   生成蓝牙数据帧
//  param:   SN   参见aLock通用协议
//  param:   Ver  
//  param:   safe  
//  
//  return： Frame 帧指针
*******************************************************/
uint8_t* ble_frame (uint8_t SN,uint8_t Ver,uint8_t safe, 
					uint8_t Reply, uint8_t DataType, uint16_t Data_Len, uint8_t* Data_P )
{
	static uint8_t Frame[40] ;
	uint8_t temp_add=0;
		
	Frame[0] = SN;
	Frame[1] = Ver;
	Frame[2] = safe << 4 | Reply;
	Frame[3] = 3;
	Frame[4] = DataType;
	Frame[5] = Data_Len >> 8;
	Frame[6] = Data_Len;

	for(uint16_t i=0; i<Data_Len; i++)
	{
		Frame[7+i] = *(Data_P++);
		temp_add = temp_add + Frame[7+i];
	}
		

	ESP_LOGD("BLE","ble data check:%02x",temp_add);
	Frame[7+Data_Len] = temp_add;
	
  
	return Frame;
}

uint8_t * Ble_get_data(uint8_t* ble_get_frame, uint8_t data_len)
{
	static uint8_t ble_data[50] = {0};
	memset(ble_data, 0, 50);
	for(uint8_t i=0; i<data_len; i++)
	{
		ble_data[i] = ble_get_frame[7+i];
	}
	return ble_data;
}


/* **************************************************
//  brief: 用于传输长度不定的蓝牙数据
//  param: gatts_if   
//  param: Data_len  数据长度
//  param: Data_p  数据指针
//  return None
*******************************************************/
void ALble_send (esp_gatt_if_t gatts_if, uint16_t Data_len, uint8_t* Data_p )
{
	uint16_t attr_handle = 0x002a;

	uint8_t data_quot = Data_len / 20;
	uint8_t data_rem =  Data_len % 20;

	if(data_quot > 0){
		for(uint8_t i=0;i<data_quot;i++){
			esp_ble_gatts_send_indicate(gatts_if, 0, attr_handle,20, Data_p , false);
			Data_p += 20;
		}
		
		esp_ble_gatts_send_indicate(gatts_if, 0, attr_handle,data_rem, Data_p , false);
	}
	else{
		esp_ble_gatts_send_indicate(gatts_if, 0, attr_handle,Data_len , Data_p , false);
	}	
}


void ble_tasksend(void *pvParameters)
{
	ABLEQueue = xQueueCreate(10, sizeof(BLEframe));        //创建数据指针接受队列
	BLEframe bleframe = {
	 	.framedata_type = Type_BLE,
		.framedata_len = 0,   
		.framedata_p = NULL,     
		.framedata_reply = 0,		
	};
	uint8_t ble_reply =0;
    for(;;){
		if(xQueueReceive(ABLEQueue, &bleframe, 0)){
			for(uint8_t i = 0; i < bleframe.framedata_len; i++)
			{
				ESP_LOGD("BLE","send data:%02x",*(uint8_t*)(bleframe.framedata_p+i));			
			}
		
			ESP_LOGD("BLE","send%d",BleSN);
			if(bleframe.framedata_reply == 1){
				ble_reply = Do_Reply;
			}
			else{
				ble_reply = No_Reply;
			}

			uint8_t *send_pack =  ble_frame (BleSN++,0,Data_Security,ble_reply,0, bleframe.framedata_len, bleframe.framedata_p );
			ALble_send(gatt_a_if , 8+bleframe.framedata_len, send_pack);	
		}
    }
	vTaskDelete(NULL);
	
}

BindRet BindEventdeal(void)
{
	uint8_t send_cnt = 0;
	uint8_t delay_cnt = 0;
	uint8_t device_id[13] = Device_id;

	while(1){
		if(BLECONNECTED == 1){
			BLEframe bleframe = {
				.framedata_type = Type_BLE,
				.framedata_len = 13,   
				.framedata_p = device_id,	  
				.framedata_reply = 1,		
			};
			
				vTaskDelay(100);
				ESP_LOGD("BLE","send device id");	
				xQueueSend(ABLEQueue,&bleframe, 0);
				
				vTaskDelay(100);
				if(BLEACK == 1){
					return BindOk;
				
				}
				else{
					send_cnt++;
					if(send_cnt >= 6)
						return BindNoAck;
					
				}		
							
		}
		else{		
			vTaskDelay(200);		// 延时2000 毫秒 
			delay_cnt++; 
			if(delay_cnt == 30)
				return BindTimeout;
		}
	
	}
	
}


void ble_taskmanage(void *pvParameters)
{
	ABLEEventGroup = xEventGroupCreate();
	if(ABLEEventGroup == NULL){
		ESP_LOGE("BLE","Creat BLEEventGroup failed");
	}
	EventBits_t uxBits;

	for(;;){	
		uxBits = xEventGroupWaitBits(
					ABLEEventGroup,		// 
					BindEevent,	   	 	// 
					pdTRUE,				// 在返回时设置位被清除
					pdFALSE,			// 不用所有位都被置位就会返回
					portMAX_DELAY );	// Wait a maximum
		ESP_LOGD("BLE", "return bit%x",uxBits);		
		if((uxBits & BindEevent) == BindEevent){
			//产生绑定事件 多次发送设备ID
			ESP_LOGD("BLE","BindStatus");
			BindRet Ret = BindEventdeal();
			if(Ret == BindTimeout){
				ESP_LOGD("BLE","BindTimeout");
			}
			else if(Ret == BindOk){
				ESP_LOGD("BLE","BindOk");
			}
			else if(Ret == BindNoAck){
				ESP_LOGD("BLE","BindNoAck");
			}
	
		}
    }
	vTaskDelete(NULL);
	
}


void Ble_data_analyexe(uint8_t * got_data,uint8_t got_data_len, esp_ble_gatts_cb_param_t *param)
{
	uint8_t data_head[5] = {0};
	static uint8_t Passwd_got = 0;
	static uint8_t Ssid_got = 0;
	ESP_LOGD("BLE","get extern order");
	for(uint8_t i=0;i<4;i++){
		data_head[i] = got_data[i];
	}
	
	static uint8_t u_ok[4] = {"U_ok"};
	BLEframe bleframe = {
			.framedata_type = Type_BLE,
			.framedata_len = 4,   
			.framedata_p = u_ok,	 
			.framedata_reply = 0,		
	};
			
	ESP_LOGD("BLE","data_head:%s",data_head);

	if(strcmp((char*)got_data,"E_Discon") == 0){
		
		esp_ble_gap_disconnect(param->write.bda);
	}
	else if(strcmp((char*)data_head,"E_S_") == 0){
		

		for(uint8_t i = 0;i<got_data_len-4;i++)
			Ble_Get_W_config.Assid[i] = got_data[4+i];

		xQueueSend(ABLEQueue,&bleframe, 0);
		ESP_LOGD("BLE","get wifi.ssid");
		Ssid_got = 1;


	}
	else if(strcmp((char*)data_head,"E_P_") == 0){

	
		for(uint8_t i = 0;i<got_data_len-4;i++)
			Ble_Get_W_config.Apasswd[i] = got_data[4+i];

		xQueueSend(ABLEQueue,&bleframe, 0);
		ESP_LOGD("BLE","get wifi.passwd");
		Passwd_got = 1;
		
	}
	else if(strcmp((char*)got_data,"E_GetIp") == 0){
		
		if(socket_created == 1){	
			static uint8_t ip_addr_send[10] = {"E_IP_"};
			for(uint8_t i = 0;i<4;i++){
				ip_addr_send[5 + i] = *(((uint8_t*)&s_ip_addr)+i);
			}
			bleframe.framedata_len = 9;
			bleframe.framedata_p = (uint8_t*)&ip_addr_send ;

			ESP_LOGD("BLE","send ip");
			xQueueSend(ABLEQueue,&bleframe, 0);

		}
		else{
			static uint8_t WifiConFaile[10] = {"E_Confail"};
			bleframe.framedata_len = 9 ;
			bleframe.framedata_p = (uint8_t*)&WifiConFaile ;

			ESP_LOGD("BLE","wifi config failed");
			xQueueSend(ABLEQueue,&bleframe, 0);
		} 

	}

	if(Passwd_got && Ssid_got == 1){
		Passwd_got = 0;
		Ssid_got = 0;	
	
		esp_err_t err = save_wifi_config( &Ble_Get_W_config );
		if(err == ESP_OK){
			ESP_LOGD("wifi","save wifi config success");
		}


		wificonfiggot = 1;     //得到数据后会重启设备
	}
	
}



