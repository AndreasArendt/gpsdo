#include "usb.h"
#include "usbd_def.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <queue.h>

#define USB_MSG_MAX_SIZE   128   // max message size per send

struct {
	float pcb_temp__C;
	uint16_t samples_delay;
} s_global_state_t;

QueueHandle_t xUsbQueue;

typedef struct {
    char msg[USB_MSG_MAX_SIZE];
    size_t len;
} UsbMessage_t;

void usb_init() {
	MX_USB_DEVICE_Init();
	xUsbQueue = xQueueCreate(10, sizeof(UsbMessage_t));
}

void usbTask(void *argument) {

	usb_printf("Startup USB");

	UsbMessage_t in;

	for (;;) {
		// Wait (block) for data from other tasks
		if (xQueueReceive(xUsbQueue, &in, portMAX_DELAY)) {
			if (in.len > 0) {
				// Transmit via USB CDC
				while (CDC_Transmit_FS((uint8_t*)in.msg, in.len) == USBD_BUSY) {
					vTaskDelay(pdMS_TO_TICKS(2));  // Wait until USB is ready
				}
			}
		}
	}
}

void usb_printf(const char *fmt, ...) {
	UsbMessage_t out;
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(out.msg, sizeof(out.msg), fmt, ap);
	va_end(ap);

	if (len < 0)
		return; // formatting error

	out.len = (len >= (int) sizeof(out.msg)) ? sizeof(out.msg) - 1 : len;

	// Non-blocking: try to send, drop if full
	xQueueSend(xUsbQueue, &out, 0);
}

void usb_printf_ISR(const char *fmt, ...) {
	UsbMessage_t out;  // same struct as used in usb_printf
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(out.msg, sizeof(out.msg), fmt, ap);
	va_end(ap);

	if (len <= 0)
		return;

	out.len = (len >= (int) sizeof(out.msg)) ? sizeof(out.msg) - 1 : len;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xQueueSendFromISR(xUsbQueue, &out, &xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
