#include "controller.h"
#include "hal.h"

#include <math.h>

#define DAC_CS_Pin GPIO_PIN_15
#define DAC_CS_GPIO_Port GPIOA

#define DAC_SYNC_PORT GPIOA
#define DAC_SYNC_PIN GPIO_PIN_15

void DAC_Select()   { HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_RESET); }
void DAC_Unselect() { HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_SET); }


#define DAC_VREF 4.096f  // adjust to your actual VREF

void DAC_AD5541A_set_value(uint16_t value)
{
	if(value >= 48000) return;

	uint8_t txData[2];

	// Split into bytes (MSB first)
	txData[0] = (value >> 8) & 0xFF;
	txData[1] = value & 0xFF;

	// Latch sequence
	HAL_GPIO_WritePin(DAC_SYNC_PORT, DAC_SYNC_PIN, GPIO_PIN_RESET);
	__NOP(); __NOP();
	HAL_SPI_Transmit(&hspi1, txData, 2, HAL_MAX_DELAY);
	__NOP(); __NOP();
	HAL_GPIO_WritePin(DAC_SYNC_PORT, DAC_SYNC_PIN, GPIO_PIN_SET);
}

void DAC_SetVoltage(float voltage)
{
    // Clamp and scale
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > 3.0f) voltage = DAC_VREF;

    uint16_t value = (uint16_t)roundf((voltage / DAC_VREF) * 65535.0f);
    DAC_AD5541A_set_value(value);
}

void controllerTask(void *argument) {

	float volt = 0.0f;
	uint16_t cnt = 0;

	while (1) {

		DAC_SetVoltage(volt);
		//usb_printf("count: %d\n", cnt);
		//DAC_AD5541A_set_value(cnt++);

		if(cnt >= 47999)
			cnt = 0;

		osDelay(100);
	}
}
