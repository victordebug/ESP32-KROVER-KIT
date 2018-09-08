#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"

#include "Yuart.h"
#include "UDPclient.h"
#include "ALble.h"
#include "Awifi.h"

#define ECHO_TEST_TXD  (17)
#define ECHO_TEST_RXD  (16)
//#define ECHO_TEST_RTS  (18)
//#define ECHO_TEST_CTS  (19)


#define BUF_SIZE (1024)
#define EX_UART_NUM UART_NUM_1

#define DATA_ERR   0x00
#define BLE_FRAME  0x01
#define UDP_FRAME  0x02
#define LOCALINFO  0x03

static const char *TAG = "uart_events";

uint8_t resetbind = 1;


/*
brief : 分类属于传递到蓝牙或WiFi的数据，同时校验校验和

return：校验通过返回数据类型，校验不通过则返回error
*/
//A queue to handle UART event.
uint8_t Uart_data_analyze(uint8_t * data, uint8_t frame_len)	
{
	uint8_t data_len =  data[2];
	uint8_t check_data = 0;
	for(uint8_t i = 0;i < data_len;i++){
		check_data = check_data + data[3+i];
	}
	ESP_LOGD("UART", "Data check：%02x", check_data);
	
	if(check_data == data[3+data_len]){
		if((data[1]&0x0F) == 0x01){
			return BLE_FRAME;
		}
		if((data[1]&0x0F) == 0x02){
			return UDP_FRAME;
		}
		if((data[1]&0x0F) == 0x03){
			return LOCALINFO;
		}	
	}
	else{
		return DATA_ERR;
	}
		
	return DATA_ERR;
}


void Uart_anlyse_exe(uint8_t * uartrcv_data_p, uint8_t uartrcv_data_len)
{
		UDPframe udpframe = {
			.framedata_type = Type_UDP,
			.framedata_len = 0,   
			.framedata_p = NULL,	 
			.framedata_reply = 0,		
		};
		BLEframe bleframe = {
			.framedata_type = Type_BLE,
			.framedata_len = 0,   
			.framedata_p = NULL,	 
			.framedata_reply = 0,		
		};
		
		ESP_LOGI(TAG, "data: %s", (char *)uartrcv_data_p);
		uint8_t Data_Type = Uart_data_analyze((uint8_t*)uartrcv_data_p,(uint8_t)uartrcv_data_len);
		if(Data_Type ==  DATA_ERR){
			ESP_LOGD("UART", "Data check error");	//数据校验错误
		}
		
		if(Data_Type == BLE_FRAME){
			ESP_LOGD("UART", "send BLE_data");
			if((*(uartrcv_data_p+1) & 0x10) == 0x10){
				bleframe.framedata_reply = 1;
			}

			bleframe.framedata_len = *(uartrcv_data_p+2);
			bleframe.framedata_p = uartrcv_data_p+3;
			BaseType_t queueret = xQueueSend(ABLEQueue,&bleframe, 0);
			if(queueret == errQUEUE_FULL){
				ESP_LOGD("UART", "queue failed");	 //入队错误
			}  
		}
		
		if(Data_Type == UDP_FRAME){
			if(socket_created == 0){
				ESP_LOGD("socket", "socket not init");
			}
			else{
				ESP_LOGD("UART", "send UDP_data");
				if((*(uartrcv_data_p+1) & 0x10) == 0x10){
					ESP_LOGD("UART", "need reply");
					udpframe.framedata_reply = 1;
				}
				
				udpframe.framedata_len = *(uartrcv_data_p+2);
				udpframe.framedata_p = uartrcv_data_p+3;
				BaseType_t queueret = xQueueSend(AUDPQueue,&udpframe, 0);
				if(queueret == errQUEUE_FULL){
					ESP_LOGD("UART", "queue failed");	 //入队错误
				}

			}

		}

		if(Data_Type == LOCALINFO){
			uint8_t order[20] = {0};			

			for(uint8_t i = 0; i < *(uartrcv_data_p+2); i++){
				order[i] =  uartrcv_data_p[3+i];
			}
			if(strcmp((char*)order,(char*)"bind") == 0){
				ESP_LOGD("UATR", "set BindEvent");
				EventBits_t uxBits = xEventGroupSetBits(ABLEEventGroup, BindEevent);
//				resetbind = 0;
				ESP_LOGD("UART", "return bit：%x",uxBits);
			}
			if(strcmp((char*)order,(char*)"reset") == 0){
				ESP_LOGD("UATR", "reset bind");
//				resetbind = 1;
			}
		}
	
	
}

static QueueHandle_t uart1_queue;

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
	uint8_t recvcnt=0;
	uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
	if(data == NULL ){
		ESP_LOGI("Yuart","malloc data buffer failed");
	}
	memset(data,0,BUF_SIZE);

	uint8_t * temp_data_p = data;            
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart1_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
#ifndef DEBUG_BY_UART			
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
#endif
			switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.
                in this example, we don't process data in event, but read data outside.*/
                case UART_DATA:
                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
#ifndef DEBUG_BY_UART			
                    ESP_LOGI(TAG, "data, len: %d; buffered len: %d", event.size, buffered_size);
#endif
					if(recvcnt == 10){
						recvcnt = 0;
						temp_data_p = data;
						memset(data,0,BUF_SIZE);
					}
					
					int len = uart_read_bytes(EX_UART_NUM, temp_data_p, event.size, 100 / portTICK_RATE_MS);
					ESP_LOGD(TAG, "data: %s", (char *)data);

					if(len > 0)
					{
						ESP_LOGD("UART","recv from uart:");
						for(uint8_t i=0;i<event.size;i++)
						{	
							ESP_LOGD("","%c",*(temp_data_p+i));
							//ESP_LOGD("UART","recv from uart:%x \ %c",*(temp_data_p+i),*(temp_data_p+i));
						}
	
						if(*temp_data_p == 0xaa )
						{
							
							Uart_anlyse_exe(temp_data_p,event.size);
							
							temp_data_p += event.size;
							recvcnt++;		
						}
						else
						{
							ESP_LOGD("UART","recv error data ");   //产生的杂波
						}
					
					}
					break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow\n");
                    //If fifo overflow happened, you should consider adding flow control for your application.
                    //We can read data out out the buffer, or directly flush the rx buffer.
                    uart_flush(EX_UART_NUM);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full\n");
                    //If buffer full happened, you should consider encreasing your buffer size
                    //We can read data out out the buffer, or directly flush the rx buffer.
                    uart_flush(EX_UART_NUM);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break\n");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error\n");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error\n");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    ESP_LOGI(TAG, "uart pattern detected\n");
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d\n", event.type);
                    break;
            }
        }
    }
    free(data);
    data = NULL;
    vTaskDelete(NULL);
}



void uart_tasksend(void *pvParameters)
{
	UARTframe  frame = {
	 	.framedata_type = 0,
		.framedata_len = 0,
		.framedata_p = NULL
	};
		
	uint8_t* UART_data = (uint8_t*)malloc(50); 	
	memset(UART_data,0,50);

	
	for(;;)
	{
		if(xQueueReceive(YuartQueue, &frame, (portTickType)portMAX_DELAY))
		{
#ifndef DEBUG_BY_UART		
			ESP_LOGD("UART:" , "recv data_len:%d",(uint32_t)frame.framedata_len );
#endif
			if(frame.framedata_type == Type_UDP){

				ESP_LOGD("UART:" , "recv from UDP" );

				uint8_t* UART_data_p = UART_data;
				
				*UART_data_p++ = 0xAA;
				if(frame.framedata_reply == 1){
					*UART_data_p++ = 0x01;
				}
				else{
					*UART_data_p++ = 0x00;
				}
				
				*UART_data_p++ = frame.framedata_len;
				
				for(uint8_t i=0; i<frame.framedata_len+1 ; i++){
					*UART_data_p++ = *(frame.framedata_p+i);
				}
				
				uart_write_bytes(EX_UART_NUM, (char*) UART_data, frame.framedata_len+4);
				
				memset(UART_data,0,50);
				memset(frame.framedata_p - 7,0,50);
			}
			else{

				ESP_LOGD("UART:" , "recv from BLE" );

				uint8_t* UART_data_p = UART_data;
				
				*UART_data_p++ = 0xAA;
				if(frame.framedata_reply == 1){
					*UART_data_p++ = 0x01;
				}
				else{
					*UART_data_p++ = 0x00;
				}
				
				*UART_data_p++ = frame.framedata_len;
				
				for(uint8_t i=0; i<frame.framedata_len+1 ; i++){
					*UART_data_p++ = *(frame.framedata_p+i);
				}
				
				uart_write_bytes(EX_UART_NUM, (char*) UART_data, frame.framedata_len+4);
				
				memset(UART_data,0,50);
				memset(frame.framedata_p - 7,0,50);
			}
			

		}
	}
	vTaskDelete(NULL);
}




void uart_init()
{
    uart_config_t uart_config = {
       .baud_rate = 115200,
       .data_bits = UART_DATA_8_BITS,
       .parity = UART_PARITY_DISABLE,
       .stop_bits = UART_STOP_BITS_1,
       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
       .rx_flow_ctrl_thresh = 122,
    };
    //Set UART parameters
    uart_param_config(EX_UART_NUM, &uart_config);
    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart1_queue, 0);

    //Set UART pins (using UART0 default pins ie no changes.)
    //uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_set_pin(EX_UART_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Set uart pattern detect function.
    uart_enable_pattern_det_intr(EX_UART_NUM, '+', 3, 10000, 10, 10);

	YuartQueue = xQueueCreate(10, sizeof(UARTframe));
	
    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
    xTaskCreate(uart_tasksend, "uart_send", 2048, NULL, 12, NULL);

}



