#include "controller.h"
#include "hal.h"
#include "led.h"
#include "filter.h"
#include "pps.h"
#include "flatbuf_message_builder.h"
#include "manager.h"
#include "gpsdo_config.h"

#include <math.h>
#include <stdbool.h>

#define DAC_CS_Pin GPIO_PIN_15
#define DAC_CS_GPIO_Port GPIOA

#define DAC_SYNC_PORT GPIOA
#define DAC_SYNC_PIN GPIO_PIN_15

static const float Kp = 0.002f;
static const float Ki = 0.00001f;
static const float Kd = 0.0f;

static KF_DebugSnapshot kf_debug = { 0 };

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

float controller_ema(float val, float prev, float alpha) {
	return alpha * val + (1 - alpha) * prev;
}

void controllerTask(void *argument) {
	while (!hal_initialized)
		osDelay(100);

	static float volt = V_Mid;
	static float freq_drift_HzDs_prev = 0.0f, prev_phase = 0.0f, prev_freq = 0.0f;
	static bool first_run = true;

	filter_init();

	while (1) {
		osSemaphoreAcquire(xPPSSemaphoreHandle, osWaitForever);
		toggle_led_orange();
		uint32_t delta = pps_get_delta();

		filter_step(delta, volt);
		float phase_cnt = filter_get_phase_count();
		float freq_off_Hz = filter_get_frequency_offset_Hz();
		float freq_drift_HzDs = filter_get_frequency_drift_HzDs();

		// ema smoothing
		if(first_run)
			first_run = false;
		else
		{
			phase_cnt = controller_ema(phase_cnt, prev_phase, 0.02);       // 50 s
			freq_off_Hz = controller_ema(freq_off_Hz, prev_freq, 0.01);    // 100 s
			freq_drift_HzDs = controller_ema(freq_drift_HzDs, freq_drift_HzDs_prev, 0.001); // 1000s
		}

		freq_drift_HzDs_prev = freq_drift_HzDs;
		prev_phase = phase_cnt;
		prev_freq = freq_off_Hz;

		volt = control(-phase_cnt, -freq_off_Hz, -freq_drift_HzDs);
		DAC_SetVoltage(volt);

		// sent flatbuf
		flatbuf_send_status(phase_cnt, freq_off_Hz, freq_drift_HzDs, volt,
				get_volt_meas(), get_temperature(), delta);
		filter_get_kf_debug_flatbuf(&kf_debug);
		flatbuf_send_kf_debug(&kf_debug);
	}
}
