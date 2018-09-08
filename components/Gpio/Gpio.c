#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"


void GPIO_Init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set 
    io_conf.pin_bit_mask = 1<<0;         //设置GPIO0   
    //disable pull-down mode
    io_conf.pull_down_en = 1;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);	

    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set 
    io_conf.pin_bit_mask = 1<<15;         //设置GPIO0   
    //disable pull-down mode
    io_conf.pull_down_en = 1;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);	

	gpio_set_level(GPIO_NUM_0, 0);       //设置camera低功耗开关关闭
	gpio_set_level(GPIO_NUM_15, 0);       //设置camera低功耗开关关闭
}


