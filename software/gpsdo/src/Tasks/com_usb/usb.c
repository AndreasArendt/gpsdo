#include "usb.h"
#include "usbd_def.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define USB_TX_BUFFER_SIZE 512   // bytes
#define USB_MSG_MAX_SIZE   128   // max message size per send

MessageBufferHandle_t usbTxMessageBuffer;

void usb_init() {
	MX_USB_DEVICE_Init();
	usbTxMessageBuffer = xMessageBufferCreate(USB_TX_BUFFER_SIZE);
}

void usbTask(void *argument) {
	uint8_t msg[USB_MSG_MAX_SIZE];
	size_t len;

	for (;;) {
		// Wait (block) for data from other tasks
		len = xMessageBufferReceive(usbTxMessageBuffer, msg, sizeof(msg),
				portMAX_DELAY);

		if (len > 0) {
			// Transmit via USB CDC
			while (CDC_Transmit_FS(msg, len) == USBD_BUSY) {
				vTaskDelay(pdMS_TO_TICKS(2));  // Wait until USB is ready
			}
		}
	}
}

void usb_printf(const char *fmt, ...) {
	char buffer[128];
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if (len < 0) {
		return; // formatting error
	}

	/* vsnprintf returns the number of characters that would have been written
	 excluding the null terminator. We want to send at most sizeof(buffer)-1. */
	size_t send_len = (size_t) (
			(len >= (int) sizeof(buffer)) ? (sizeof(buffer) - 1) : len);

	/* Non-blocking: wait 0 ticks. You can change to portMAX_DELAY if you want to block. */
	xMessageBufferSend(usbTxMessageBuffer, buffer, send_len, 0);
}

void usb_printf_ISR(const char *fmt, ...) {
	char buffer[64]; /* keep small: ISR stack is limited */
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if (len <= 0) {
		return;
	}

	size_t send_len = (size_t) (
			(len >= (int) sizeof(buffer)) ? (sizeof(buffer) - 1) : len);

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xMessageBufferSendFromISR(usbTxMessageBuffer, buffer, send_len,
			&xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
