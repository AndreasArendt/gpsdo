/*
 * manager.c
 *
 *  Created on: Oct 25, 2025
 *      Author: andia
 */

#include "hal.h"
#include "usb.h"

#include <math.h>

// taken from: https://viereck.ch/ntc/
float ntc_calc_temperature(uint16_t adc_value) {
	float inputVoltage = 3.3;
	float r = 10;
	float ntcR25 = 10;
	float ntcBeta = 3435;
	float minVoltage = 0.000;
	float maxVoltage = 3.300;
	float maxValue = 4096;

	float measuredVoltage = adc_value / maxValue * (maxVoltage - minVoltage)
			+ minVoltage;
	float rNtc = measuredVoltage * r / (inputVoltage - measuredVoltage);
	float temperature__C = 1
			/ (logf(rNtc / ntcR25) / ntcBeta + 1 / (273.15 + 25)) - 273.15;

	return temperature__C;
}

void mangerTask(void *argument) {
	for (;;) {

		uint16_t temperature_raw = HAL_ADC_GetValue(&hadc1); // get the adc value
		float temperature__C = ntc_calc_temperature(temperature_raw);

		usb_printf("Temperature: %d.%02d\n", (int) temperature__C,
				fabs((int) ((temperature__C - (int) temperature__C) * 100)));

		osDelay(1000);
	}
}

