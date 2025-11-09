#include "usb.h"
#include "usbd_core.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
#include "assert.h"
#include "flatbuf_defs.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <queue.h>

/* --- FreeRTOS objects --- */
QueueHandle_t xUsbTxQueue = NULL;
static QueueHandle_t xUsbRxQueue = NULL;
static QueueSetHandle_t usbQueueSet = NULL;

/* Diagnostic counters */
static volatile uint32_t usb_tx_dropped = 0;
static volatile uint32_t usb_rx_dropped = 0;

/* Extern USB device handle (provided by CubeMX USB stack) */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Forward declarations */
static void usb_wait_enumerated(void);
static void usb_wait_tx_complete(void);

/* Initialize USB subsystem and queues */
void usb_init(void) {
    /* Start USB stack */
    MX_USB_DEVICE_Init();

    /* Create queues (must be created before usbTask runs) */
    xUsbTxQueue = xQueueCreate(USB_TX_QUEUE_LENGTH, sizeof(flatbuf_message_t));
    xUsbRxQueue = xQueueCreate(USB_RX_QUEUE_LENGTH, sizeof(flatbuf_message_t));
    configASSERT(xUsbTxQueue);
    configASSERT(xUsbRxQueue);

    /* Create queue set (optional; used by usbTask) */
    usbQueueSet = xQueueCreateSet(USB_QUEUE_SET_LENGTH);
    configASSERT(usbQueueSet);

    /* Add queues to the queue set */
    /* xQueueAddToSet returns errQUEUE_FULL if usbQueueSet not big enough - assert */
    BaseType_t added;
    added = xQueueAddToSet(xUsbTxQueue, usbQueueSet);
    configASSERT(added == pdPASS);
    added = xQueueAddToSet(xUsbRxQueue, usbQueueSet);
    configASSERT(added == pdPASS);
}

/* The system USB task: serializes TX and handles RX */
void usbTask(void *argument) {
    flatbuf_message_t in;
    QueueSetMemberHandle_t activeQueue;

    /* Wait until USB stack enumerated before doing any transfers */
    usb_wait_enumerated();

    for (;;) {
        /* Wait for either TX or RX queue to have data */
        activeQueue = xQueueSelectFromSet(usbQueueSet, portMAX_DELAY);

        if (activeQueue == xUsbTxQueue) {
            if (xQueueReceive(xUsbTxQueue, &in, 0) == pdTRUE) {
                /* Compute total message size: header + payload */
                size_t total_len = sizeof(in.magic) + sizeof(in.msg_id) + sizeof(in.len) + in.len;
                uint8_t *buf = (uint8_t *)&in;  // points to the start of the struct (header)

                size_t offset = 0;
                while (offset < total_len) {
                    size_t chunk = total_len - offset;
                    if (chunk > USB_EP_MPS) chunk = USB_EP_MPS;

                    /* Ensure device still configured */
                    usb_wait_enumerated();

                    /* Try to transmit with a reasonable retry policy */
                    uint8_t res;
                    uint32_t attempts = 0;
                    do {
                        res = CDC_Transmit_FS(&buf[offset], chunk);
                        if (res == USBD_OK) break;
                        /* USBD_BUSY: wait a bit and retry */
                        vTaskDelay(pdMS_TO_TICKS(2));
                        attempts++;
                    } while (attempts < 500); /* ~1s retry budget */

                    /* If still not OK, drop remaining and count */
                    if (res != USBD_OK) {
                        usb_tx_dropped++;
                        break;
                    }

                    /* Wait until TxState goes to 0 (transfer completed) */
                    usb_wait_tx_complete();

                    /* Advance */
                    offset += chunk;
                }
            }
        } else if (activeQueue == xUsbRxQueue) {
            if (xQueueReceive(xUsbRxQueue, &in, 0) == pdTRUE) {
                /* Application-specific: handle received data */
                // example: echo back
                xQueueSend(xUsbTxQueue, &in, 0);
            }
        } else {
            /* Unexpected queue set member - ignore */
        }
    }
}


/* Wait until the USB device is configured/enumerated by the host */
static void usb_wait_enumerated(void) {
    uint32_t timeout_ms = 5000;
    uint32_t waited = 0;
    while (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
        vTaskDelay(pdMS_TO_TICKS(10));
        waited += 10;
        if (waited >= timeout_ms) {
            /* Give up waiting after timeout - break to avoid deadlock */
            break;
        }
    }
}

/* Wait for transmit completion by checking internal TxState.
 * This is safe because only usbTask() will call CDC_Transmit_FS().
 */
static void usb_wait_tx_complete(void) {
    /* Access CDC internal handle if available */
    if (hUsbDeviceFS.pClassData == NULL) {
        /* Not configured or class data not initialized yet */
        return;
    }

    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;

    /* TxState: 0 = ready, 1 = busy (internal to the CDC class) */
    uint32_t wait_count = 0;
    while (hcdc->TxState != 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++wait_count > 2000) { /* ~2s safety cap */
            break;
        }
    }
}

/* Optional diagnostics retrieval (call from a debug console) */
void usb_get_diagnostics(uint32_t *tx_drops, uint32_t *rx_drops) {
    if (tx_drops) *tx_drops = usb_tx_dropped;
    if (rx_drops) *rx_drops = usb_rx_dropped;
}

