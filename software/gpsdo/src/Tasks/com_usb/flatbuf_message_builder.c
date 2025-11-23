/*
 * flatbuf_message_builder.c
 *
 *  Created on: Nov 9, 2025
 *      Author: andia
 */
#include "flatbuf_message_builder.h"
#include "flatbuf_flatcc_alloc.h"
#include "gpsdo_builder.h"
#include "flatcc_builder.h"
#include "cmsis_os.h"
#include "hal.h"
#include "usb.h"
#include "flatbuf_defs.h"
#include <string.h>

#define FB_KF_BUF_SIZE     4096
#define FB_STATUS_BUF_SIZE  512

static uint8_t fb_status_out[FB_STATUS_BUF_SIZE];
static uint8_t fb_kf_out[FB_KF_BUF_SIZE];

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

void flatbuf_send_kf_debug(const KF_DebugSnapshot *kf) {
	// Select the dedicated static 16 KB arena for KF debug messages
	flatbuf_select_kf_arena();

	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	/* ----------------------------------------------------
	 * Build Vec3 x
	 * ---------------------------------------------------- */
	gpsdo_Vec3_start(&builder);
	gpsdo_Vec3_v_add(&builder, kf->x[0]);
	gpsdo_Vec3_v_add(&builder, kf->x[1]);
	gpsdo_Vec3_v_add(&builder, kf->x[2]);
	gpsdo_Vec3_ref_t x_ref = gpsdo_Vec3_end(&builder);

	/* ----------------------------------------------------
	 * Build Mat3x3 P
	 * ---------------------------------------------------- */
	gpsdo_Mat3x3_start(&builder);
	for (int i = 0; i < 9; i++) {
		gpsdo_Mat3x3_m_add(&builder, kf->P[i]);
	}
	gpsdo_Mat3x3_ref_t P_ref = gpsdo_Mat3x3_end(&builder);

	/* ----------------------------------------------------
	 * Build kf_state_debug
	 * ---------------------------------------------------- */
	gpsdo_kf_state_debug_ref_t state_ref = gpsdo_kf_state_debug_create(&builder,
			x_ref, P_ref, kf->x[2]); // drift

	/* ----------------------------------------------------
	 * Build correction block
	 * ---------------------------------------------------- */
	gpsdo_kf_correction_debug_ref_t corr_ref = gpsdo_kf_correction_debug_create(
			&builder, kf->z, kf->h_x, kf->y, kf->S, kf->mahal_d2, kf->nis,
			kf->rejected);

	/* ----------------------------------------------------
	 * Build K (3x1)
	 * ---------------------------------------------------- */
	gpsdo_Mat3x1_start(&builder);
	gpsdo_Mat3x1_m_add(&builder, kf->K[0]);
	gpsdo_Mat3x1_m_add(&builder, kf->K[1]);
	gpsdo_Mat3x1_m_add(&builder, kf->K[2]);
	gpsdo_Mat3x1_ref_t K_ref = gpsdo_Mat3x1_end(&builder);

	/* ----------------------------------------------------
	 * Build H (1x3)
	 * ---------------------------------------------------- */
	gpsdo_Mat1x3_start(&builder);
	gpsdo_Mat1x3_m_add(&builder, kf->H[0]);
	gpsdo_Mat1x3_m_add(&builder, kf->H[1]);
	gpsdo_Mat1x3_m_add(&builder, kf->H[2]);
	gpsdo_Mat1x3_ref_t H_ref = gpsdo_Mat1x3_end(&builder);

	/* ----------------------------------------------------
	 * Build Q (3x3)
	 * ---------------------------------------------------- */
	gpsdo_Mat3x3_start(&builder);
	for (int i = 0; i < 9; i++) {
		gpsdo_Mat3x3_m_add(&builder, kf->Q[i]);
	}
	gpsdo_Mat3x3_ref_t Q_ref = gpsdo_Mat3x3_end(&builder);

	/* ----------------------------------------------------
	 * Build main kf_debug table
	 * ---------------------------------------------------- */
	gpsdo_kf_debug_ref_t dbg_ref = gpsdo_kf_debug_create(&builder,
			kf->timestamp_s, state_ref, corr_ref, K_ref, H_ref, Q_ref, kf->R,
			kf->outlier_count, kf->iteration);

	/* ----------------------------------------------------
	 * Build Message with union payload
	 * ---------------------------------------------------- */
	gpsdo_Message_start_as_root(&builder);
	gpsdo_Message_timestamp_s_add(&builder, kf->timestamp_s);

	gpsdo_Payload_union_ref_t ure = gpsdo_Payload_as_kf_debug(dbg_ref);
	gpsdo_Message_payload_add_value(&builder, ure);
	gpsdo_Message_payload_add_type(&builder, ure.type);

	gpsdo_Message_end_as_root(&builder);

	/* ----------------------------------------------------
	 * Retrieve FlatBuffer directly (no malloc)
	 * ---------------------------------------------------- */
	size_t msg_size;
	const uint8_t *buf = flatcc_builder_get_direct_buffer(&builder, &msg_size);

	if (msg_size <= FB_KF_BUF_SIZE) {
		memcpy(fb_kf_out, buf, msg_size);
		flatbuf_send(FLATBUF_MSG_KF_DEBUG, fb_kf_out, msg_size);
	}

	flatcc_builder_clear(&builder);
}

void flatbuf_send_status(float phase_cnt, float freq_error, float freq_drift,
		float vctrl, float vmeas, float temp, uint32_t raw_counter_value) {
	// Select the small static arena for simple Status messages
	flatbuf_select_status_arena();

	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	/* ----------------------------------------------------
	 * Build Status table
	 * ---------------------------------------------------- */
	gpsdo_Status_ref_t status = gpsdo_Status_create(&builder, phase_cnt,
			freq_error, freq_drift, vctrl, vmeas, temp, raw_counter_value);

	/* ----------------------------------------------------
	 * Build Message root
	 * ---------------------------------------------------- */
	gpsdo_Message_start_as_root(&builder);
	gpsdo_Message_timestamp_s_add(&builder, 0.0);

	gpsdo_Payload_union_ref_t ure = gpsdo_Payload_as_Status(status);
	gpsdo_Message_payload_add_value(&builder, ure);
	gpsdo_Message_payload_add_type(&builder, ure.type);

	gpsdo_Message_end_as_root(&builder);

	/* ----------------------------------------------------
	 * Retrieve FlatBuffer from builder’s static arena
	 * ---------------------------------------------------- */
	size_t msg_size;
	const uint8_t *buf = flatcc_builder_get_direct_buffer(&builder, &msg_size);

	if (msg_size <= FB_STATUS_BUF_SIZE) {
		memcpy(fb_status_out, buf, msg_size);
		flatbuf_send(FLATBUF_MSG_STATUS, fb_status_out, msg_size);
	}

	flatcc_builder_clear(&builder);
}
