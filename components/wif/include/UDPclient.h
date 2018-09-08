#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#define DATA_ERROR        0x00

#define Data_Security     0x00
#define Data_encryption1  0x01
#define Data_encryption2  0x02
#define No_Reply          0x00
#define Do_Reply          0x01

typedef struct _UDPframe{
	uint8_t  framedata_type;
	uint8_t  framedata_reply;
	uint16_t framedata_len;
	uint8_t *framedata_p;
}UDPframe;



QueueHandle_t AUDPQueue;

void Init_UDPSocket(void);
void AUDP_send(void *pvParameters);
void AUDP_recv(void *pvParameters);
uint8_t* UDP_frame (uint8_t SN,uint8_t Ver,uint8_t safe, 
					uint8_t Reply, uint8_t DataType, uint16_t Data_Len, uint8_t* Data_P );
#endif