/*
 * pps.c
 *
 *  Created on: Oct 18, 2025
 *      Author: andia
 */

#include "pps.h"
#include "hal.h"
#include "usb.h"
#include "led.h"

static uint32_t last_PPS = 0;

void pps_init() {
	HAL_TIM_Base_Start(&htim2);   // start high word first
	HAL_TIM_Base_Start(&htim1);   // then low word (counts 625kHz)

	HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_1);
}

static uint32_t Read625kHzCount(void) {
	uint16_t low;
	uint32_t high;

	// ensure coherent read
	do {
		high = TIM2->CNT;
		low = TIM1->CNT;
	} while (high != TIM2->CNT);

	return (high << 16) | low;
}

char msg[32];
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM5) {
		uint32_t now = Read625kHzCount();
		toggle_led_orange();

		if (last_PPS != 0) {
			uint32_t delta =
					(now > last_PPS) ? (now - last_PPS) : (last_PPS - now);

			usb_printf_ISR("delta: %lu\r\n", (unsigned long)delta);
		}

		last_PPS = now;
	}
}
