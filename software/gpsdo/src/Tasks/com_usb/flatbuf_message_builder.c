/*
 * flatbuf_message_builder.c
 *
 *  Created on: Nov 9, 2025
 *      Author: andia
 */
#include "flatbuf_message_builder.h"
#include "gpsdo_builder.h"
#include "flatcc_builder.h"
#include "cmsis_os.h"
#include "hal.h"
#include "usb.h"

#define USB_TX_BUF_SIZE 128

void flatbuf_send_status(float freq_error, float freq_drift, float vctrl, float vmeas, float temp)
{
    flatcc_builder_t builder;
    flatcc_builder_init(&builder);

    // create the message
    gpsdo_Status_start_as_root(&builder);
    gpsdo_Status_freq_error_Hz_add(&builder, freq_error);
    gpsdo_Status_freq_dirft_HzDs_add(&builder, freq_drift);
    gpsdo_Status_voltage_control_V_add(&builder, vctrl);
    gpsdo_Status_voltage_measured_V_add(&builder, vmeas);
    gpsdo_Status_temperature_C_add(&builder, temp);
    gpsdo_Status_end_as_root(&builder);

    size_t msg_size = flatcc_builder_get_buffer_size(&builder);

    UsbMessage_t msg;
    if (msg_size > sizeof(msg.data)) {
        // message too large — drop or handle error
        flatcc_builder_clear(&builder);
        return;
    }

    flatcc_builder_copy_buffer(&builder, msg.data, msg_size);
    msg.len = msg_size;

    // Send to USB TX queue
    xQueueSend(xUsbTxQueue, &msg, 0);

    flatcc_builder_clear(&builder);
}
