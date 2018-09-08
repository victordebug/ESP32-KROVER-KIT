#ifndef JPEGRTPDEAL_H
#define JPEGRTPDEAL_H

#include "lwip/err.h"

EventGroupHandle_t PicEventGroup;              //任务事件组
#define PicSendEevent        (1<<0)


void InitJpegSocket();
void SendPic ();
err_t JpegSocketConnect();
err_t JpegSocketClose();
void PicSend(void *pvParameters);

#endif 