/*
 * manager.c
 *
 *  Created on: Oct 25, 2025
 *      Author: andia
 */

#include "hal.h"
#include "usb.h"
#include "pps.h"

#include <math.h>

#define ADC_VREF 3.3f
#define ADC_MAX_VALUE 4096.0f

// taken from: https://viereck.ch/ntc/
float ntc_calc_temperature(uint16_t adc_value) {

	float r = 10e3;
	float ntcR25 = 10e3;
	float ntcBeta = 3425;

	float measuredVoltage = adc_value / ADC_MAX_VALUE * ADC_VREF;
	float rNtc = measuredVoltage * r / (ADC_VREF - measuredVoltage);
	float temperature__C = 1
			/ (logf(rNtc / ntcR25) / ntcBeta + 1 / (273.15 + 25)) - 273.15;

	return temperature__C;
}

// calculate control voltage for oscillator
float vref_ocxo_calc(uint16_t adc_value){
	return (adc_value / ADC_MAX_VALUE) * ADC_VREF;
}

void mangerTask(void *argument) {
	pps_init();

	hal_initialized = 1;

	for (;;) {

		uint16_t ch1 = adc_dma_buffer[0];
		uint16_t ch3 = adc_dma_buffer[1];

		float voltage__V = vref_ocxo_calc(ch1);
		int voltage_frac = fabsf((int) ((voltage__V - (int) voltage__V) * 100.0f));

		float temperature__C = ntc_calc_temperature(ch3);
		int temperature_frac = fabsf((int) ((temperature__C - (int) temperature__C) * 100.0f));

		usb_printf("Temperature: %d.%d\r\n", (int) temperature__C, temperature_frac);
		usb_printf("Voltage: %d.%d\r\n", (int) voltage__V, voltage_frac);

		osDelay(1000);
	}
}

