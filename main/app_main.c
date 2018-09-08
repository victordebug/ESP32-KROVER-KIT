#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"
#include "Agatt_server.h"
#include "Yuart.h"
#include "Awifi.h"
#include "UDPclient.h"
#include "ALble.h"
#include "period_task.h"
#include "Gpio.h"
#include "camera.h"
#include "JpegRTPDeal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "camera_lcd.h"
#include "sdkconfig.h"



static const char * TAG = "app_main";
extern uint8_t* BLE_Recv_Buffer;

void vApplicationIdleHook( void )
{
	for(;;);
}


extern "C" void app_init()
{
    nvs_flash_init();
	GPIO_Init();
	uart_init();
 	ble_init();
	esp_err_t wifi_init_err = initialise_wifi();
	
#if CONFIG_USE_LCD
    app_lcd_init();
#endif

	wifi_camera_init();

#if CONFIG_USE_LCD
	lcd_camera_init_complete();
	lcd_init_wifi();
#endif

	ESP_LOGI("app_main", "Free heap: %u", xPortGetFreeHeapSize());

	uint8_t* BLE_recv_data = (uint8_t*)malloc(50);	
	memset(BLE_recv_data,0,50);
	BLE_Recv_Buffer = BLE_recv_data;

	ESP_LOGI("app_main", "creat blesend task ");
	BaseType_t task_ret = xTaskCreate(&ble_tasksend, "BLE_send", 2048, NULL, 5, NULL);
	if(task_ret != pdPASS)
	{
		ESP_LOGE("app_main", "creat bletask failed");
	}
	
	ESP_LOGI("app_main", "creat blemanage task ");
	BaseType_t taskmanage_ret = xTaskCreate(&ble_taskmanage, "ble_taskmanage", 2048, NULL, 5, NULL);
	if(taskmanage_ret != pdPASS)
	{		
		ESP_LOGE("app_main", "creat blemanagetask failed");
	}

	ESP_LOGI("app_main", "creat period task ");
	xTaskCreate(&period_task, "period_task", 2048, NULL, 5, NULL);

	ESP_LOGI("app_main", "creat PicSend task ");
	xTaskCreate(&PicSend, "PicSend", 2048, NULL, 9, NULL);


	if(wifi_init_err == ESP_OK)
	{
		ESP_LOGI("app_main", "creat UDP_recv task ");
		xTaskCreate(&AUDP_recv, "AUDP_recv", 2048, NULL, 5, NULL);		

		ESP_LOGI("app_main", "creat UDP_send task ");
		xTaskCreate(&AUDP_send, "AUDP_send", 2048, NULL, 8, NULL);	
	}

#if CONFIG_USE_LCD
		ESP_LOGD(TAG, "Starting app_lcd_task...");
		xTaskCreatePinnedToCore(&app_lcd_task, "app_lcd_task", 4096, NULL, 4, NULL, 0);
#endif


	ESP_LOGI("app_main", "creat task end ");

}

