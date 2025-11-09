/*
 * flatbuf_defs.h
 *
 *  Created on: Nov 9, 2025
 *      Author: andia
 */

#ifndef TASKS_COM_USB_FLATBUF_DEFS_H_
#define TASKS_COM_USB_FLATBUF_DEFS_H_

#pragma once

#define FLATBUF_MAGIC      0xB00B  // or whatever sync you use

typedef enum {
    FLATBUF_MSG_STATUS = 1,
} flatbuf_msg_id_t;

typedef struct __attribute__((packed))
{
	uint16_t magic;
	uint16_t msg_id;
	uint16_t len;
	uint8_t data[USB_MSG_MAX_SIZE];
} flatbuf_message_t;

#endif /* TASKS_COM_USB_FLATBUF_DEFS_H_ */
