/*
 * filter.h
 *
 *  Created on: Nov 7, 2025
 *      Author: andia
 */

#ifndef FILTER_FILTER_H_
#define FILTER_FILTER_H_

#include <stdint.h>
#include <stdbool.h>

void filter_init(void);
void filter_predict(float voltage_ctrl);
void filter_correct(float delta);

void filter_step(float delta, float voltage_ctrl);

bool filter_pre_check(float delta);
float filter_ema(float x, float prev_y, float alpha);

float filter_get_frequency_offset_Hz();
float filter_get_frequency_drift_HzDs();

#endif /* FILTER_FILTER_H_ */
