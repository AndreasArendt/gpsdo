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
#include <string.h>
#include <stdlib.h>

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
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// ----------------------------------------------------
	// Build Vec3 x
	// ----------------------------------------------------
	gpsdo_Vec3_start(&builder);
	gpsdo_Vec3_v_add(&builder, kf->x[0]);
	gpsdo_Vec3_v_add(&builder, kf->x[1]);
	gpsdo_Vec3_v_add(&builder, kf->x[2]);
	gpsdo_Vec3_ref_t x_ref = gpsdo_Vec3_end(&builder);

	// ----------------------------------------------------
	// Build Mat3x3 P
	// ----------------------------------------------------
	gpsdo_Mat3x3_start(&builder);
	for (int i = 0; i < 9; i++) {
		gpsdo_Mat3x3_m_add(&builder, kf->P[i]);
	}
	gpsdo_Mat3x3_ref_t P_ref = gpsdo_Mat3x3_end(&builder);

	// ----------------------------------------------------
	// Build state block (kf_state_debug)
	//   table kf_state_debug {
	//     x: Vec3;
	//     p: Mat3x3;
	//     drift: float;
	//   }
	// ----------------------------------------------------
	gpsdo_kf_state_debug_ref_t state_ref = gpsdo_kf_state_debug_create(&builder,
			x_ref, P_ref, kf->x[2]); // drift = x[2]

	// ----------------------------------------------------
	// Correction block (kf_correction_debug)
	//   table kf_correction_debug {
	//     z: float;
	//     h_x: float;
	//     y: float;
	//     s: float;
	//     mahalanobis_d2: float;
	//     nis: float;
	//     rejected: bool;
	//   }
	// ----------------------------------------------------
	gpsdo_kf_correction_debug_ref_t corr_ref = gpsdo_kf_correction_debug_create(
			&builder, kf->z, kf->h_x, kf->y, kf->S,        // maps to field 's'
			kf->mahal_d2, kf->nis, kf->rejected);

	// ----------------------------------------------------
	// Kalman Gain K (Mat3x1)
	// ----------------------------------------------------
	gpsdo_Mat3x1_start(&builder);
	gpsdo_Mat3x1_m_add(&builder, kf->K[0]);
	gpsdo_Mat3x1_m_add(&builder, kf->K[1]);
	gpsdo_Mat3x1_m_add(&builder, kf->K[2]);
	gpsdo_Mat3x1_ref_t K_ref = gpsdo_Mat3x1_end(&builder);

	// ----------------------------------------------------
	// H (Mat1x3)
	// ----------------------------------------------------
	gpsdo_Mat1x3_start(&builder);
	gpsdo_Mat1x3_m_add(&builder, kf->H[0]);
	gpsdo_Mat1x3_m_add(&builder, kf->H[1]);
	gpsdo_Mat1x3_m_add(&builder, kf->H[2]);
	gpsdo_Mat1x3_ref_t H_ref = gpsdo_Mat1x3_end(&builder);

	// ----------------------------------------------------
	// Q (Mat3x3)
	// ----------------------------------------------------
	gpsdo_Mat3x3_start(&builder);
	for (int i = 0; i < 9; i++) {
		gpsdo_Mat3x3_m_add(&builder, kf->Q[i]);
	}
	gpsdo_Mat3x3_ref_t Q_ref = gpsdo_Mat3x3_end(&builder);

	// ----------------------------------------------------
	// Build kf_debug table
	//   table kf_debug {
	//     timestamp_s: double;
	//     state: kf_state_debug;
	//     correction: kf_correction_debug;
	//     k: Mat3x1;
	//     h: Mat1x3;
	//     q: Mat3x3;
	//     r: float;
	//     outlier_count: uint32;
	//     kf_iteration: uint32;
	//   }
	// ----------------------------------------------------
	gpsdo_kf_debug_ref_t kf_ref = gpsdo_kf_debug_create(&builder,
			kf->timestamp_s, state_ref, corr_ref, K_ref, H_ref, Q_ref, kf->R,
			kf->outlier_count, kf->iteration);

	// ----------------------------------------------------
	// Wrap into Message root with union Payload { Status, kf_debug }
	//   table Message {
	//     timestamp_s: double;
	//     payload: Payload;
	//   }
	// ----------------------------------------------------
	gpsdo_Message_start_as_root(&builder);
	gpsdo_Message_timestamp_s_add(&builder, kf->timestamp_s);

	// Union payload: kf_debug
	gpsdo_Payload_union_ref_t ure = gpsdo_Payload_as_kf_debug(kf_ref);
	gpsdo_Message_payload_add_value(&builder, ure);
	gpsdo_Message_payload_add_type(&builder, ure.type);

	gpsdo_Message_end_as_root(&builder);

	// ----------------------------------------------------
	// Send binary buffer
	// ----------------------------------------------------
	size_t msg_size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &msg_size);

	flatbuf_send(FLATBUF_MSG_KF_DEBUG, (const uint8_t*) buf, msg_size);
	free(buf);

	flatcc_builder_clear(&builder);
}

void flatbuf_send_status(float phase_cnt, float freq_error, float freq_drift,
		float vctrl, float vmeas, float temp, uint32_t raw_counter_value) {
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Build Status object
	gpsdo_Status_ref_t status = gpsdo_Status_create(&builder, phase_cnt,
			freq_error, freq_drift, vctrl, vmeas, temp, raw_counter_value);

	// Wrap Status into Payload union
	gpsdo_Payload_union_ref_t ure = gpsdo_Payload_as_Status(status);

	// Build Message root
	gpsdo_Message_start_as_root(&builder);

	gpsdo_Message_timestamp_s_add(&builder, 0.0);  // or real timestamp

	// Union payload
	gpsdo_Message_payload_add_value(&builder, ure);
	gpsdo_Message_payload_add_type(&builder, ure.type);

	gpsdo_Message_end_as_root(&builder);

	// Transmit buffer
	size_t msg_size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &msg_size);

	flatbuf_send(FLATBUF_MSG_STATUS, (const uint8_t*) buf, msg_size);
	free(buf);

	flatcc_builder_clear(&builder);
}

