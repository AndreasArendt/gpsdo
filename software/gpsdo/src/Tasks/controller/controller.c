#include "controller.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;

void controllerTask(void *argument) {

	HAL_TIM_Base_Start(&htim2);   // start high word first
	HAL_TIM_Base_Start(&htim1);   // then low word (counts 625kHz)

	HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_1);

	while (1) {
		osDelay(1);
	}
}

uint32_t Read625kHzCount(void)
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

volatile uint32_t last_PPS = 0;

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

        	asm("nop");
        }
        last_PPS = now;
    }
}
