/*
 * filter.c
 *
 * Created on: Nov 7, 2025
 * Author: andia
 */

#include "filter.h"
#include "gpsdo_config.h"
#include <arm_math.h>
#include <string.h>

static const float T = 1.0f;
static const float R = ((F_OSC_HZ / EXPECTED_CTR) * (F_OSC_HZ / EXPECTED_CTR))
		/ 12.0f;
static const uint32_t MAX_JITTER = 2u;

// State & matrices storage
static float F_data[9] = { 1.0f, T,    0.5*T*T,
		                   0.0f, 1.0f, T,
                           0.0f, 0.0f, 1.0f};
static float P_data[9] = { 10.0f,  0.0f,  0.0f,
                            0.0f, 10.0f,  0.0f,
                            0.0f,  0.0f, 10.0f};
static float Q_data[9] = { 0 };
static float H_data[3] = { 1.0f, 0.0f, 0.0f };
static float X_data[3] = { 0.0f, 0.0f, 0.0f };
static float X_pred_data[3] = { 0.0f, 0.0f, 0.0f };

static float FT_data[9];
static float temp_3x3a_data[9];
static float temp_3x3b_data[9];
static float temp_3x1a_data[3];
static float temp_3x1b_data[3];
static float temp_1x3_data[3];
static float K_data[3];
static float S_data[1];
static float I_data[9] = { 1.0f, 0.0f, 0.0f,
		                   0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 1.0f};

static arm_matrix_instance_f32 F, FT, P, Q, H, HT;
static arm_matrix_instance_f32 X, X_pred;
static arm_matrix_instance_f32 K;          // 3x1
static arm_matrix_instance_f32 S;          // 1x1
static arm_matrix_instance_f32 temp_3x3a;
static arm_matrix_instance_f32 temp_3x3b;
static arm_matrix_instance_f32 temp_3x1a;
static arm_matrix_instance_f32 temp_3x1b;
static arm_matrix_instance_f32 temp_1x3;
static arm_matrix_instance_f32 I;

static inline void mat_copy_f32(const arm_matrix_instance_f32 *src,
		arm_matrix_instance_f32 *dst) {
	memcpy(dst->pData, src->pData,
			src->numRows * src->numCols * sizeof(float32_t));
}

void filter_init(void) {
	// Q Matrix & Q-scaling
	float q = 1e-7;
	Q_data[0] = q * powf(T,5)/20.0f;
	Q_data[1] = q * powf(T,4)/8.0f;
	Q_data[2] = q * powf(T,3)/6.0f;
	Q_data[3] = q * powf(T,4)/8.0f;
	Q_data[4] = q * powf(T,3)/3.0f;
	Q_data[5] = q * powf(T,2)/2.0f;
	Q_data[6] = q * powf(T,3)/6.0f;
	Q_data[7] = q * powf(T,2)/2.0f;
	Q_data[8] = q * T;

	// initialize matrix
	arm_mat_init_f32(&F, 3, 3, F_data);
	arm_mat_init_f32(&FT, 3, 3, FT_data);
	arm_mat_init_f32(&P, 3, 3, P_data);
	arm_mat_init_f32(&Q, 3, 3, Q_data);
	arm_mat_init_f32(&H, 1, 3, H_data);
	arm_mat_init_f32(&HT, 3, 1, H_data);
	arm_mat_init_f32(&X, 3, 1, X_data);
	arm_mat_init_f32(&X_pred, 3, 1, X_pred_data);
	arm_mat_init_f32(&K, 3, 1, K_data);
	arm_mat_init_f32(&S, 1, 1, S_data);

	arm_mat_init_f32(&temp_3x3a, 3, 3, temp_3x3a_data);
	arm_mat_init_f32(&temp_3x3b, 3, 3, temp_3x3b_data);
	arm_mat_init_f32(&temp_3x1a, 3, 1, temp_3x1a_data);
	arm_mat_init_f32(&temp_3x1b, 3, 1, temp_3x1b_data);
	arm_mat_init_f32(&temp_1x3, 1, 3, temp_1x3_data);

	arm_mat_init_f32(&I, 3, 3, I_data);

	// pre-computations
	arm_mat_trans_f32(&F, &FT);

	// zeros for temps
	memset(temp_3x3a_data, 0, sizeof(temp_3x3a_data));
	memset(temp_3x3b_data, 0, sizeof(temp_3x3b_data));
	memset(temp_3x1a_data, 0, sizeof(temp_3x1a_data));
	memset(temp_3x1b_data, 0, sizeof(temp_3x1b_data));
	memset(temp_1x3_data, 0, sizeof(temp_1x3_data));
	memset(K_data, 0, sizeof(K_data));
	memset(S_data, 0, sizeof(S_data));
}

void filter_predict(float voltage_ctrl) {
	// X_pred = F * X
	arm_mat_mult_f32(&F, &X, &X_pred);

	// X_pred = X_pred + Bu (very implicit!)
	//X_pred.pData[0] += (voltage_ctrl - V_Mid) * Ku_HzDV;

	// P = F * P * FT + Q
	arm_mat_mult_f32(&F, &P, &temp_3x3a); // FP
	arm_mat_mult_f32(&temp_3x3a, &FT, &temp_3x3b);
	arm_mat_add_f32(&temp_3x3b, &Q, &P);

	// optional
	mat_copy_f32(&X_pred, &X); // copy result back in X
}

// exponential moving average
float filter_ema(float x, float prev_y, float alpha) {
    return alpha * x + (1 - alpha) * prev_y;
}

// return true, if value is considered good
bool filter_pre_check(float delta) {
	return ((delta <= (EXPECTED_CTR + MAX_JITTER))
			&& (delta >= (EXPECTED_CTR - MAX_JITTER)));
}

void filter_correct(float delta) {
	// Convert count delta to frequency deviation
	const float scale = (F_OSC_HZ / EXPECTED_CTR);
	float delta_f = (delta - EXPECTED_CTR) * scale;

	static float total_phase_error = 0.0f;
	total_phase_error += delta_f * T;  // integrate frequency to get phase

	// y = z - H * X_pred (optimized)
	float y = total_phase_error - X_pred.pData[0];

	// S = H * P * HT + R
	arm_mat_mult_f32(&H, &P, &temp_1x3);  // HP
	arm_mat_mult_f32(&temp_1x3, &HT, &S); // HPHT
	S.pData[0] += R;                      // HPHP+R

	float S_inv = 1.0f / S.pData[0];

	// K = P * HT * S_inv
	arm_mat_mult_f32(&P, &HT, &temp_3x1a);    // PHT
	arm_mat_scale_f32(&temp_3x1a, S_inv, &K); // PHT * S_inv

	// X = X + Ky
	arm_mat_scale_f32(&K, y, &temp_3x1a);        // Ky
	arm_mat_add_f32(&X_pred, &temp_3x1a, &temp_3x1a); // X + Ky

	// P = (I - KH)P
	arm_mat_mult_f32(&K, &H, &temp_3x3a);         // KH
	arm_mat_sub_f32(&I, &temp_3x3a, &temp_3x3b);  // I-KH
	arm_mat_mult_f32(&temp_3x3b, &P, &temp_3x3a); // (I-KH)P

	// copy results
	mat_copy_f32(&temp_3x1a, &X); // copy result back in X
	mat_copy_f32(&temp_3x3a, &P); // copy result back in P
}

void filter_step(float delta, float voltage_ctrl)
{
	static float filtered_delta = 0.0f;
	filter_predict(voltage_ctrl);

	if (filtered_delta == 0.0f)
		filtered_delta = delta;

	// do correction only if considered healthy
	if(filter_pre_check(delta))
	{
		filtered_delta = filter_ema(delta, filtered_delta, 0.3);
		filter_correct(filtered_delta);
	}
}

/* Accessors */
float filter_get_phase_count() {
	return X.pData[0];
}

float filter_get_frequency_offset_Hz() {
	return X.pData[1];
}

float filter_get_frequency_drift_HzDs() {
	return X.pData[2];
}
