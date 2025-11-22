/*
 * flatbuf_defs.h
 *
 *  Created on: Nov 9, 2025
 *      Author: andia
 */

#ifndef TASKS_COM_USB_FLATBUF_DEFS_H_
#define TASKS_COM_USB_FLATBUF_DEFS_H_

#pragma once

#include "usb.h"
#include <stdbool.h>

#define FLATBUF_MAGIC      0xB00B

typedef enum {
    FLATBUF_MSG_STATUS = 1,
	FLATBUF_MSG_KF_DEBUG = 2,
} flatbuf_msg_id_t;

typedef struct __attribute__((packed))
{
	uint16_t magic;
	uint16_t msg_id;
	uint16_t len;
	uint8_t data[USB_MSG_MAX_SIZE];
} flatbuf_message_t;

typedef struct {
    double timestamp_s;

    // State
    float x[3];
    float P[9];

    // Correction
    float z;
    float h_x;
    float y;
    float S;
    float mahal_d2;
    float nis;
    bool  rejected;

    // Matrices
    float K[3];
    float H[3];
    float Q[9];
    float R;

    // Diagnostics
    uint32_t outlier_count;
    uint32_t iteration;
} KF_DebugSnapshot;


#endif /* TASKS_COM_USB_FLATBUF_DEFS_H_ */
