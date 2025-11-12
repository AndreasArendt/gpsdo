/*
 * gpsdo_config.h
 *
 *  Created on: Nov 10, 2025
 *      Author: andia
 */

#ifndef GPSDO_CONFIG_H_
#define GPSDO_CONFIG_H_

#define DAC_VREF 4.096f

// OXCO frequency
static const float F_OSC_HZ = 10.0e6f;

// OCXO downsampling counts
static const float EXPECTED_CTR = 625000.0f;

// DAC min/max voltages
static const float V_Max = 3.0;
static const float V_Min = 0.5;
static const float V_Mid = 2.0;

// OCXO tuning range (ppm at voltage)
static const float PPM_AT_4V =  1.5f;
static const float PPM_AT_0V = -1.5f;

// Hertz per Volt
//static const float Ku_HzDV = (((F_OSC_HZ / 1.0e6f) * PPM_AT_4V) - ((F_OSC_HZ / 1.0e6f) * PPM_AT_0V)) / 4.0f;
static const float Ku_HzDV = 0.75f;

#endif /* GPSDO_CONFIG_H_ */
