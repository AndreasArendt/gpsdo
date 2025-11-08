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

#include <semphr.h>

static volatile uint32_t last_PPS = 0;
static volatile uint32_t PPS_delta = 0;

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

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    BaseType_t xHigherPriorityTaskWoken;

	if (htim->Instance == TIM5) {
		uint32_t now = Read625kHzCount();

		toggle_led_orange();

		if (last_PPS != 0) {
			PPS_delta = now - last_PPS;

            xSemaphoreGiveFromISR(xPPSSemaphoreHandle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			//osSemaphoreRelease(xPPSSemaphoreHandle);
		}

		last_PPS = now;
	}
}

uint32_t pps_get_delta() {
	return PPS_delta;
}
