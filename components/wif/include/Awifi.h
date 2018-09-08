#ifndef AWIFI_H
#define AWIFI_H

#include "lwip/ip_addr.h"


#define WIFI_NOT_SET -1
extern uint8_t socket_created;
extern ip4_addr_t s_ip_addr;

typedef struct _Awifi_config{
	char Assid[40];
	char Apasswd[40];	
}Awifi_config;

void wifi_init_task(void *pvParameters);
esp_err_t initialise_wifi();
void Ainitialise_wifi(Awifi_config *Wifi_config );
esp_err_t get_wifi_config(Awifi_config* wifi_cp );
esp_err_t save_wifi_config(Awifi_config* wifi_cp);




#endif



