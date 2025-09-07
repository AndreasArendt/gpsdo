#include "controller.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#include <math.h>

extern I2C_HandleTypeDef hi2c1;

#define AD5693R_ADDR (0x4C << 1)

void controllerTask(void *argument)
{
	/* Infinite loop */
	for (;;) {
		while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
			asm volatile("nop");
		}

		for (float angle = 0; angle <= 2 * M_PI; angle += 0.1) {
			uint16_t value = (uint16_t) ((sinf(angle) + 1) * 32767.5);

			// Convert to two bytes for transmission
			uint8_t data[3];
			data[0] = 0x30; // Command = 0011 (Write to DAC), Addr = 0000 → 0x30
			data[1] = (uint8_t) (value >> 8);   // MSB
			data[2] = (uint8_t) (value & 0xFF); // LSB

			HAL_I2C_Master_Transmit_DMA(&hi2c1, AD5693R_ADDR, data, 3);

			while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
				asm volatile("nop");
			}
		}

		osDelay(1);
	}
}
