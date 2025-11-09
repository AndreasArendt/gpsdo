#ifndef TASK_USB_H_
#define TASK_USB_H_

#include "hal.h"

/* --- CONFIGURATION --- */
#define USB_EP_MPS            64     // Full-Speed endpoint max packet size
#define USB_MSG_MAX_SIZE      USB_EP_MPS
#define USB_TX_QUEUE_LENGTH   16
#define USB_RX_QUEUE_LENGTH   8
#define USB_QUEUE_SET_LENGTH  (USB_TX_QUEUE_LENGTH + USB_RX_QUEUE_LENGTH + 2)

extern MessageBufferHandle_t usbTxMessageBuffer;
extern QueueHandle_t xUsbTxQueue;

// Interface definition
void usb_init();

void usb_get_diagnostics(uint32_t *tx_drops, uint32_t *rx_drops);

#endif
