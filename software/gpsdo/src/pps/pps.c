/*
 * pps.c
 *
 *  Created on: Oct 18, 2025
 *      Author: andia
 */

#include "pps.h"
#include "hal.h"

static uint32_t last_PPS = 0;

void pps_init()
{
	HAL_TIM_Base_Start(&htim2);   // start high word first
	HAL_TIM_Base_Start(&htim1);   // then low word (counts 625kHz)

	HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_1);
}

static uint32_t Read625kHzCount(void)
{
    uint16_t low;
    uint32_t high;

    // ensure coherent read
    do {
        high = TIM2->CNT;
        low  = TIM1->CNT;
    } while (high != TIM2->CNT);

    return (high << 16) | low;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM5)
    {
        uint32_t now = Read625kHzCount();
        if (last_PPS != 0)
        {
        	uint32_t delta = 0;
        	if(now > last_PPS)
        		delta = now - last_PPS;
        	else
        		delta = last_PPS - now;

            //CDC_Transmit_FS((uint8_t*)msg, strlen(msg));

        	asm("nop");
        }
        last_PPS = now;
    }
}
