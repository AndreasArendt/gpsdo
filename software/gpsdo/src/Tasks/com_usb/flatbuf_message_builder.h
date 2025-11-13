/*
 * flatbuf_message_builder.h
 *
 *  Created on: Nov 9, 2025
 *      Author: andia
 */

#ifndef TASKS_COM_USB_FLATBUF_MESSAGE_BUILDER_H_
#define TASKS_COM_USB_FLATBUF_MESSAGE_BUILDER_H_

#include <stdint.h>

void flatbuf_send_status(float phase_cnt, float freq_error, float freq_drift, float vctrl, float vmeas, float temp, uint32_t raw_counter_value);

#endif /* TASKS_COM_USB_FLATBUF_MESSAGE_BUILDER_H_ */
