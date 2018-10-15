#ifndef _CAMERA_LCD_H_
#define _CAMERA_LCD_H_


#ifdef __cplusplus             //告诉编译器，这部分代码按C语言的格式进行编译，而不是C++的
extern "C"
{
#endif



//LCD
#define CONFIG_USE_LCD	          0
#define CONFIG_HW_LCD_MISO_GPIO   25
#define CONFIG_HW_LCD_MOSI_GPIO	  23
#define CONFIG_HW_LCD_CLK_GPIO    19
#define CONFIG_HW_LCD_CS_GPIO     22
#define CONFIG_HW_LCD_DC_GPIO     21
#define CONFIG_HW_LCD_RESET_GPIO  18
#define CONFIG_HW_LCD_BL_GPIO     5


#if   CONFIG_USE_LCD

void queue_send(uint8_t frame_num);
uint8_t queue_receive();
uint8_t queue_available();
void app_lcd_task(void *pvParameters);
void lcd_init_wifi(void);
void lcd_camera_init_complete(void);
void lcd_wifi_connect_complete(void);
//void lcd_http_info(ip4_addr_t s_ip_addr);
void app_lcd_init();

#endif//end CONFIG_USE_LCD


#ifdef __cplusplus
}
#endif   //end of __cpluscplus


#endif


