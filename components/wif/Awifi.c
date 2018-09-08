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

uint8_t socket_created = 0;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
ip4_addr_t s_ip_addr;
uint8_t connect_cut = 3;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	
	ESP_LOGD("wifi", "event_id:%d",event->event_id);
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:      //2
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:     //7
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            s_ip_addr = event->event_info.got_ip.ip_info.ip;
			
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:         //5
            /* This is a workaround as ESP32 WiFi libs don't currently
             auto-reassociate. */

			if(connect_cut == 0){              //wifi连接只会在重启后发生
				esp_wifi_stop();
				esp_wifi_disconnect();
			}

			if(connect_cut > 0){
				connect_cut-- ;
				esp_wifi_connect();
            	xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
			}

			break;
        default:
            break;
    }
    return ESP_OK;
}



esp_err_t initialise_wifi()
{

		Awifi_config Wifi_config={
			.Assid = {0},
			.Apasswd = {0},
		};

		esp_err_t wifi_config_err = get_wifi_config( &Wifi_config );
		ESP_LOGD("wifi", "wifi not init err %d", wifi_config_err);

		if(1){//wifi_config_err == ESP_OK
			ESP_LOGD("wifi", "ssid %s",Wifi_config.Assid);
			ESP_LOGD("wifi", "passwd %s",Wifi_config.Apasswd);
			
		    tcpip_adapter_init();

		    wifi_event_group = xEventGroupCreate();

		    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

		    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

		    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
			wifi_config_t wifi_config = {
				.sta = {
					.ssid = CONFIG_WIFI_SSID,
					.password = CONFIG_WIFI_PASSWORD,
				},
			};

			for(uint8_t i =0; i<strlen(Wifi_config.Assid); i++){
				wifi_config.sta.ssid[i] = (uint8_t)Wifi_config.Assid[i];		
			}
			for(uint8_t i =0; i<strlen(Wifi_config.Apasswd); i++){
				wifi_config.sta.password[i] = (uint8_t)Wifi_config.Apasswd[i]; 	
			}
		
			ESP_LOGD("wifi", "WIFIssid %s",wifi_config.sta.ssid);
			ESP_LOGD("wifi", "WIFIpasswd %s",wifi_config.sta.password);	
			
			ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
			ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
			ESP_ERROR_CHECK( esp_wifi_start() );
			ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_NONE) );
			
			ESP_LOGI("wifi", "Connecting to \"%s\"", wifi_config.sta.ssid);
			EventBits_t bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 1000);
			if(bits == CONNECTED_BIT){

				ESP_LOGI("wifi", "Connected");

				if(socket_created == 0){
					Init_UDPSocket();
					socket_created = 1;				
				}
				InitJpegSocket();
				return ESP_OK;
			}
			return WIFI_NOT_SET;
		}
		else{
			ESP_LOGI("wifi", "wifi not set");
			return WIFI_NOT_SET;
		}
		
		
}



#define STORAGE_NAMESPACE "Wifi_config"
extern Awifi_config Ble_Get_W_config;
nvs_handle wifi_handle;

esp_err_t save_wifi_config(Awifi_config* wifi_cp)
{
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &wifi_handle);
    if (err != ESP_OK) 
		return err;
     
    err = nvs_set_blob(wifi_handle, "wifi_config", wifi_cp, sizeof(Awifi_config));
    if (err != ESP_OK) 
		return err;
/*
	* After setting any values, nvs_commit() must be called to ensure changes are written
	* to non-volatile storage. Individual implementations may write to storage at other times,
	* but this is not guaranteed.
*/
    err = nvs_commit(wifi_handle);
    if (err != ESP_OK) 
		return err;

    // Close
//    nvs_close(wifi_handle);
    return ESP_OK;
}


esp_err_t get_wifi_config(Awifi_config* wifi_cp )
{
	esp_err_t err;	

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &wifi_handle);
    if (err != ESP_OK) 
		return err;
	
    size_t required_size = sizeof(Awifi_config);  
    err = nvs_get_blob(wifi_handle, "wifi_config", wifi_cp, &required_size);
    if (err != ESP_OK) 
		return err;

	return ESP_OK;
}


