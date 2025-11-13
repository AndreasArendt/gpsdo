#include "controller.h"
#include "hal.h"
#include "led.h"
#include "filter.h"
#include "pps.h"
#include "flatbuf_message_builder.h"
#include "manager.h"
#include "gpsdo_config.h"

#include <math.h>

#define DAC_CS_Pin GPIO_PIN_15
#define DAC_CS_GPIO_Port GPIOA

#define DAC_SYNC_PORT GPIOA
#define DAC_SYNC_PIN GPIO_PIN_15

static const float Kp = 0.0045f;
static const float Ki = 0.0000245f;
static const float Kd = 0.000225f;

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
	if (voltage < V_Min)
		voltage = V_Min;
	if (voltage > V_Max)
		voltage = V_Max;

	uint16_t value = (uint16_t) roundf((voltage / DAC_VREF) * 65535.0f);
	DAC_AD5541A_set_value(value);
}

float control(float phase_cnt, float freq_offset, float freq_drift) {
	float p = Kp * freq_offset;
	float i = Ki * phase_cnt;
	float d = Kd * freq_drift;

	float v_out = V_Mid + p + i + d;

	// clamping
	if (v_out > V_Max)
		v_out = V_Max;
	if (v_out < V_Min)
		v_out = V_Min;

	return v_out;
}

void controllerTask(void *argument) {
	while (!hal_initialized)
		osDelay(100);

	static float volt = V_Mid;

	filter_init();

	while (1) {
		osSemaphoreAcquire(xPPSSemaphoreHandle, osWaitForever);
		toggle_led_orange();
		uint32_t delta = pps_get_delta();

		filter_step(delta, volt);
		float phase_cnt = filter_get_phase_count();
		float freq_off_Hz = filter_get_frequency_offset_Hz();
		float freq_drift_HzDs = filter_get_frequency_drift_HzDs();

		//volt = control(-freq_off_Hz, -freq_drift_HzDs, 1.0f);
		DAC_SetVoltage(volt);

		// sent flatbuf
		flatbuf_send_status(phase_cnt, freq_off_Hz, freq_drift_HzDs, volt, get_volt_meas(), get_temperature(), delta);
	}
}
