#include "usb.h"
#include "usbd_def.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <queue.h>

#define USB_MSG_MAX_SIZE      128   // max message size per send
#define USB_QUEUE_SET_LENGTH  8

struct {
	float pcb_temp__C;
	uint16_t samples_delay;
} s_global_state_t;

typedef struct {
	uint8_t data[USB_MSG_MAX_SIZE];
	size_t len;
} UsbMessage_t;

static QueueHandle_t xUsbTxQueue;
static QueueHandle_t xUsbRxQueue;

static QueueSetHandle_t usbQueueSet;

void usb_init() {
	MX_USB_DEVICE_Init();
	xUsbTxQueue = xQueueCreate(5, sizeof(UsbMessage_t));
	xUsbRxQueue = xQueueCreate(5, sizeof(UsbMessage_t));
}

void usbTask(void *argument) {
	UsbMessage_t in;
	QueueSetMemberHandle_t activeQueue;

	// Create a queue set that can hold events from both queues
	usbQueueSet = xQueueCreateSet(USB_QUEUE_SET_LENGTH);
	xQueueAddToSet(xUsbTxQueue, usbQueueSet);
	xQueueAddToSet(xUsbRxQueue, usbQueueSet);

	usb_printf("Startup USB");

	for (;;) {
		// Wait for data from either queue
		activeQueue = xQueueSelectFromSet(usbQueueSet, portMAX_DELAY);

		if (activeQueue == xUsbTxQueue) {
			if (xQueueReceive(xUsbTxQueue, &in, 0)) {
				while (CDC_Transmit_FS((uint8_t*) in.data, in.len) == USBD_BUSY)
					vTaskDelay(pdMS_TO_TICKS(2));
			}
		} else if (activeQueue == xUsbRxQueue) {
			if (xQueueReceive(xUsbRxQueue, &in, 0)) {
				asm("nop");// Handle received data
			}
		}
	}
}

void usb_receive_isr(uint8_t *pbuf, uint32_t *Len) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	UsbMessage_t packet;
	packet.len = *Len;
	memcpy(packet.data, pbuf, packet.len);

	xQueueSendFromISR(xUsbRxQueue, &packet, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void usb_printf(const char *fmt, ...) {
	UsbMessage_t out;
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf((char*)out.data, sizeof(out.data), fmt, ap);
	va_end(ap);

	if (len < 0)
		return; // formatting error

	out.len = (len >= (int) sizeof(out.data)) ? sizeof(out.data) - 1 : len;

	// Non-blocking: try to send, drop if full
	xQueueSend(xUsbTxQueue, &out, 0);
}

void usb_printf_ISR(const char *fmt, ...) {
	UsbMessage_t out;  // same struct as used in usb_printf
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf((char*)out.data, sizeof(out.data), fmt, ap);
	va_end(ap);

	if (len <= 0)
		return;

	out.len = (len >= (int) sizeof(out.data)) ? sizeof(out.data) - 1 : len;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xQueueSendFromISR(xUsbTxQueue, &out, &xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
