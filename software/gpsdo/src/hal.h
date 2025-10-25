/*
 * hal.h
 *
 *  Created on: Oct 18, 2025
 *      Author: andia
 */

#ifndef HAL_H_
#define HAL_H_

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "message_buffer.h"
#include "usbd_def.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;

extern ADC_HandleTypeDef hadc1;

extern USBD_HandleTypeDef hUsbDeviceFS;

#endif /* HAL_H_ */
