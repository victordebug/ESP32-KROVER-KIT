#ifndef ALBLE_H
#define ALBLE_H

#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "freertos/event_groups.h"

#define Data_Security     0x00
#define Data_encryption1  0x01
#define Data_encryption2  0x02
#define No_Reply          0x00
#define Do_Reply          0x01



typedef enum{
	BindOk,
	BindNoAck,
	BindTimeout,
}BindRet;



#define BindEevent        (1<<0)

typedef struct _BLEframe{
	uint8_t  framedata_type;
	uint8_t  framedata_reply;
	uint16_t framedata_len;
	uint8_t *framedata_p;
}BLEframe;

extern uint8_t BLECONNECTED;
extern uint8_t BLEACK;
extern uint8_t wificonfiggot;



QueueHandle_t ABLEQueue;                        //消息队列句柄
EventGroupHandle_t ABLEEventGroup;              //Ble任务事件�?



extern uint8_t BleSN;
extern uint8_t gatt_a_if;


uint8_t* ble_frame (uint8_t SN,uint8_t Ver,uint8_t safe, 
					uint8_t Reply, uint8_t DataType, uint16_t Data_Len, uint8_t* Data_P );

void ALble_send (esp_gatt_if_t gatts_if, uint16_t Data_len, uint8_t* Data_p );
void ble_tasksend(void *pvParameters);

uint8_t * Ble_get_data(uint8_t* ble_get_frame, uint8_t data_len);
void Ble_data_analyexe(uint8_t * got_data,uint8_t got_data_len, esp_ble_gatts_cb_param_t *param);
void ble_taskmanage(void *pvParameters);


#endif
