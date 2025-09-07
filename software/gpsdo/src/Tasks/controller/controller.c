#include "controller.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#include <math.h>

extern I2C_HandleTypeDef hi2c1;

#define AD5693R_ADDR (0x4C << 1)

typedef enum
{
	GAIN_1 = 0,
	GAIN_2 = 1
} e_ad5693_gain;

void AD5693_set_gain(I2C_HandleTypeDef* i2c, e_ad5693_gain gain)
{
	uint8_t data[3];
	data[0] = 0x40;  // Command: Write to DAC & Update
	data[1] = gain << 3;
	data[2] = 0x00;

	HAL_I2C_Master_Transmit_DMA(i2c, AD5693R_ADDR, data, 3);
}


// mask values for control register bits in data[1]
#define CTRL_REF_BIT   (1 << 4)  // DB12 -> bit4 of data[1]
#define CTRL_GAIN_BIT  (1 << 3)  // DB11 -> bit3 of data[1]

HAL_StatusTypeDef AD5693_set_gain_blocking(I2C_HandleTypeDef *hi2c, uint8_t enable_gain2, uint8_t enable_internal_ref)
{
    uint8_t data[3];

    data[0] = 0x40;        // Command: Write to Control Register
    data[1] = 0x00;        // D15..D8
    data[2] = 0x00;        // D7..D0

    // set REF bit if you want internal ref on
    if (enable_internal_ref) {
        data[1] |= CTRL_REF_BIT;
    }

    // set GAIN bit (DB11) for gain = 2
    if (enable_gain2) {
        data[1] |= CTRL_GAIN_BIT;
    }

    // Transmit and wait (blocking)
    return HAL_I2C_Master_Transmit_DMA(hi2c, AD5693R_ADDR, data, 3);
}

void test_gain_toggle(void)
{
    HAL_StatusTypeDef st;
    uint8_t dac[3];

    // 1) Ensure internal reference or external is as you expect.
    //    For testing, use internal ref OFF if you want to use external VREF, or ON to use internal 2.5V.
    // Example: use internal ref ON (so VREF = 2.5V)
    st = AD5693_set_gain_blocking(&hi2c1, 0 /*gain1*/, 1 /*internal ref on*/);
    if (st != HAL_OK) { /* handle error, return */ }

    HAL_Delay(10); // give it a moment

    // 2) write a test code (e.g., half-scale)
    uint16_t code = 0x8000; // mid-scale (approx 1.25V with 2.5V ref)
    dac[0] = 0x30; // Write to DAC & Update (DAC A)
    dac[1] = (uint8_t)(code >> 8);
    dac[2] = (uint8_t)(code & 0xFF);

    st = HAL_I2C_Master_Transmit_DMA(&hi2c1, AD5693R_ADDR, dac, 3);
    if (st != HAL_OK) { /* handle error */ }
    HAL_Delay(50);
    // measure Vout here (multimeter or scope) -> should be ~ Vref * code/65535

    // 3) Now set gain = 2 (DB11 = 1)
    st = AD5693_set_gain_blocking(&hi2c1, 1 /*gain2*/, 1 /*keep internal ref on*/);
    if (st != HAL_OK) { /* handle error */ }
    HAL_Delay(10);

    // 4) write same DAC code again (some devices require updating DAC after control change)
    st = HAL_I2C_Master_Transmit_DMA(&hi2c1, AD5693R_ADDR, dac, 3);
    if (st != HAL_OK) { /* handle error */ }
    HAL_Delay(50);
    // measure Vout again -> should be roughly double (or clipped at VDD)
}

void controllerTask(void *argument)
{
	AD5693_set_gain(&hi2c1, GAIN_2);

	/* Infinite loop */
	for (;;) {
		while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
			asm volatile("nop");
		}

		//test_gain_toggle();


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
