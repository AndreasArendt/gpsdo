#ifndef TASK_USB_H_
#define TASK_USB_H_

#include "hal.h"

extern MessageBufferHandle_t usbTxMessageBuffer;

void usb_init();

void usb_receive_isr(uint8_t* pbuf, uint32_t *Len);

void usb_printf(const char *fmt, ...);
void usb_printf(const char *fmt, ...);

#endif
