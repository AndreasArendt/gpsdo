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
void filter_predict(void);
void filter_correct(uint32_t delta);

bool filter_pre_check(uint32_t delta);

float filter_get_frequency_offset_Hz();
float filter_get_frequency_drift_HzDs();

#endif /* FILTER_FILTER_H_ */
