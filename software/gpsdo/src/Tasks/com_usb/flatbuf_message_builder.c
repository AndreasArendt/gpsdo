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
#include "flatbuf_defs.h"

static void flatbuf_send(uint16_t msg_id, const uint8_t *payload,
		size_t payload_len) {
	if (payload_len > sizeof(((flatbuf_message_t*) 0)->data))
		return; // too large

	flatbuf_message_t msg;
	msg.magic = FLATBUF_MAGIC;
	msg.msg_id = msg_id;
	msg.len = payload_len;
	memcpy(msg.data, payload, payload_len);

	// Send out (e.g., to USB)
	xQueueSend(xUsbTxQueue, &msg, 0);
}

void flatbuf_send_status(float freq_error, float freq_drift, float vctrl,
		float vmeas, float temp, uint32_t raw_counter_value) {
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	gpsdo_Status_start_as_root(&builder);
	gpsdo_Status_freq_error_hz_add(&builder, freq_error);
	gpsdo_Status_freq_drift_hz_s_add(&builder, freq_drift);
	gpsdo_Status_voltage_control_v_add(&builder, vctrl);
	gpsdo_Status_voltage_measured_v_add(&builder, vmeas);
	gpsdo_Status_temperature_c_add(&builder, temp);
	gpsdo_Status_raw_counter_value_add(&builder, raw_counter_value);
	gpsdo_Status_end_as_root(&builder);

	size_t msg_size = flatcc_builder_get_buffer_size(&builder);
	const void *buf = flatcc_builder_get_direct_buffer(&builder, &msg_size);

	flatbuf_send(FLATBUF_MSG_STATUS, buf, msg_size);

	flatcc_builder_clear(&builder);
}
