#include "controller.h"
#include "hal.h"
#include "led.h"
#include "filter.h"
#include "pps.h"
#include "usb.h"

#include <math.h>

#define DAC_CS_Pin GPIO_PIN_15
#define DAC_CS_GPIO_Port GPIOA

#define DAC_SYNC_PORT GPIOA
#define DAC_SYNC_PIN GPIO_PIN_15

#define DAC_VREF 4.096f

void DAC_Select() {
	HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_RESET);
}
void DAC_Unselect() {
	HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_SET);
}

void DAC_AD5541A_set_value(uint16_t value) {
	if (value >= 48000)
		return;

	uint8_t txData[2];

	// Split into bytes (MSB first)
	txData[0] = (value >> 8) & 0xFF;
	txData[1] = value & 0xFF;

	// Latch sequence
	HAL_GPIO_WritePin(DAC_SYNC_PORT, DAC_SYNC_PIN, GPIO_PIN_RESET);
	__NOP();
	__NOP();
	HAL_SPI_Transmit(&hspi1, txData, 2, HAL_MAX_DELAY);
	__NOP();
	__NOP();
	HAL_GPIO_WritePin(DAC_SYNC_PORT, DAC_SYNC_PIN, GPIO_PIN_SET);
}

void DAC_SetVoltage(float voltage) {
	// Clamp and scale
	if (voltage < 0.0f)
		voltage = 0.0f;
	if (voltage > 3.0f)
		voltage = DAC_VREF;

	uint16_t value = (uint16_t) roundf((voltage / DAC_VREF) * 65535.0f);
	DAC_AD5541A_set_value(value);
}

void controllerTask(void *argument) {
	while (!hal_initialized)
		osDelay(100);

	float volt = 0.0f;
	filter_init();
	DAC_SetVoltage(volt);

	while (1) {
		osSemaphoreAcquire(xPPSSemaphoreHandle, osWaitForever);
		uint32_t delta = pps_get_delta();

		if(delta > 0)
		{
			filter_step(delta);
			usb_printf("delta: %lu\r\n", (unsigned long) delta);

			float freq_off = filter_get_frequency_offset_Hz();
			int freq_off_frac = fabsf(
					(int) ((freq_off - (int) freq_off) * 1000.0f));

			usb_printf("freq_off: %d.%d\r\n", (int) freq_off, freq_off_frac);
		}
	}
}
