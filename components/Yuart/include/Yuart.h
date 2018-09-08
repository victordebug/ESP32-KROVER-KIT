#ifndef UART_H
#define UART_H

#define DEBUG_BY_UART


typedef enum {
    addfng,         /*!< Ìí¼ÓÖ¸ÎÆ*/
    delfng,         /*!< É¾³ýÖ¸ÎÆ*/
    forcefng,       /*!< ÉèÖÃÐ²ÆÈÖ¸ÎÆ*/
    addpwd,         /*!< Ìí¼ÓÃÜÂë*/
    delpwd,         /*!< É¾³ýÃÜÂë*/
    addic,          /*!< Ìí¼ÓIC¿¨*/
    delic,          /*!< É¾³ýIC¿¨*/
    banpwd,         /*!< ½ûÓÃÃÜÂë */
    banfng,         /*!< ½ûÓÃÖ¸ÎÆ*/ 
    banic,          /*!< ½ûÓÃÖ¸ÎÆ*/
	clearall,       /*!< ½ûÓÃÖ¸ÎÆ*/
	disablefng,     /*!< ½ûÓÃÖ¸ÎÆ*/
	disablepwd,
	disableic,
	banXid,
	forcepwd,
	setrtc,
} order_send_event_type_t;

typedef struct {
    order_send_event_type_t type; /*!< UART event type */
    size_t size;            /*!< UART data size for UART_DATA event*/
} order_send_event_t;

#define Type_UDP 0x01
#define Type_BLE 0x02

typedef struct _UARTframe{
	uint8_t  framedata_type;
	uint8_t  framedata_reply;
	uint16_t framedata_len;
	uint8_t *framedata_p;
}UARTframe;


void uart_init();
void echo_task();


QueueHandle_t YuartQueue;
extern uint8_t resetbind;


#endif
