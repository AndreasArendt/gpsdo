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
static float F_data[4] = { 1.0f, T, 0.0f, 1.0f };
static float P_data[4] = { 10.0f, 0.0f, 0.0f, 1.0f };
static float Q_data[4] = { 0.00001f, 0.0f, 0.0f, 1.0e-9f };
static float H_data[2] = { 1.0f, 0.0f };
static float X_data[2] = { 0.0f, 0.0f };
static float X_pred_data[2] = { 0.0f, 0.0f };

static float FT_data[4];
static float temp_2x2a_data[4];
static float temp_2x2b_data[4];
static float temp_2x1a_data[2];
static float temp_2x1b_data[2];
static float temp_1x2_data[2];
static float K_data[2];
static float S_data[1];
static float I_data[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

static arm_matrix_instance_f32 F, FT, P, Q, H, HT;
static arm_matrix_instance_f32 X, X_pred;
static arm_matrix_instance_f32 K;          // 2x1
static arm_matrix_instance_f32 S;          // 1x1
static arm_matrix_instance_f32 temp_2x2a;  // temporary
static arm_matrix_instance_f32 temp_2x2b;
static arm_matrix_instance_f32 temp_2x1a;
static arm_matrix_instance_f32 temp_2x1b;
static arm_matrix_instance_f32 temp_1x2;
static arm_matrix_instance_f32 I;

static inline void mat_copy_f32(const arm_matrix_instance_f32 *src,
		arm_matrix_instance_f32 *dst) {
	memcpy(dst->pData, src->pData,
			src->numRows * src->numCols * sizeof(float32_t));
}

void filter_init(void) {
	// initialize matrix
	arm_mat_init_f32(&F, 2, 2, F_data);
	arm_mat_init_f32(&FT, 2, 2, FT_data);
	arm_mat_init_f32(&P, 2, 2, P_data);
	arm_mat_init_f32(&Q, 2, 2, Q_data);
	arm_mat_init_f32(&H, 1, 2, H_data);
	arm_mat_init_f32(&HT, 2, 1, H_data);
	arm_mat_init_f32(&X, 2, 1, X_data);
	arm_mat_init_f32(&X_pred, 2, 1, X_pred_data);
	arm_mat_init_f32(&K, 2, 1, K_data);
	arm_mat_init_f32(&S, 1, 1, S_data);

	arm_mat_init_f32(&temp_2x2a, 2, 2, temp_2x2a_data);
	arm_mat_init_f32(&temp_2x2b, 2, 2, temp_2x2b_data);
	arm_mat_init_f32(&temp_2x1a, 2, 1, temp_2x1a_data);
	arm_mat_init_f32(&temp_2x1b, 2, 1, temp_2x1b_data);
	arm_mat_init_f32(&temp_1x2, 1, 2, temp_1x2_data);

	arm_mat_init_f32(&I, 2, 2, I_data);

	// pre-computations
	arm_mat_trans_f32(&F, &FT);

	// zeros for temps
	memset(temp_2x2a_data, 0, sizeof(temp_2x2a_data));
	memset(temp_2x2b_data, 0, sizeof(temp_2x2b_data));
	memset(temp_2x1a_data, 0, sizeof(temp_2x1a_data));
	memset(temp_2x1b_data, 0, sizeof(temp_2x1b_data));
	memset(temp_1x2_data, 0, sizeof(temp_1x2_data));
	memset(K_data, 0, sizeof(K_data));
	memset(S_data, 0, sizeof(S_data));
}

void filter_predict(void) {
	// X_pred = F * X
	arm_mat_mult_f32(&F, &X, &X_pred);

	// P = F * P * FT + Q
	arm_mat_mult_f32(&F, &P, &temp_2x2a); // FP
	arm_mat_mult_f32(&temp_2x2a, &FT, &temp_2x2b);
	arm_mat_add_f32(&temp_2x2b, &Q, &P);
}

// exponential moving average
float filter_ema(float x, float prev_y, float alpha) {
    return alpha * x + (1 - alpha) * prev_y;
}

// return true, if value is considered good
bool filter_pre_check(uint32_t delta) {
	return ((delta <= (EXPECTED_CTR + MAX_JITTER))
			&& (delta >= (EXPECTED_CTR - MAX_JITTER)));
}

void filter_correct(float delta) {
	// Convert count delta to frequency deviation
	const float scale = (F_OSC_HZ / EXPECTED_CTR);
	float z = (delta - EXPECTED_CTR) * scale;

	// y = z - H * X_pred (optimized)
	float y = z - X_pred.pData[0];

	// S = H * P * HT + R
	arm_mat_mult_f32(&H, &P, &temp_1x2);  // HP
	arm_mat_mult_f32(&temp_1x2, &HT, &S); // HPHT
	S.pData[0] += R;                      // HPHP+R

	float S_inv = 1.0f / S.pData[0];

	// K = P * HT * S_inv
	arm_mat_mult_f32(&P, &HT, &temp_2x1a);    // PHT
	arm_mat_scale_f32(&temp_2x1a, S_inv, &K); // PHT * S_inv

	// X = X + Ky
	arm_mat_scale_f32(&K, y, &temp_2x1a);        // Ky
	arm_mat_add_f32(&X, &temp_2x1a, &temp_2x1a); // X + Ky

	// P = (I - KH)P
	arm_mat_mult_f32(&K, &H, &temp_2x2a);         // KH
	arm_mat_sub_f32(&I, &temp_2x2a, &temp_2x2b);  // I-KH
	arm_mat_mult_f32(&temp_2x2b, &P, &temp_2x2a); // KH

	// copy results
	mat_copy_f32(&temp_2x1a, &X); // copy result back in X
	mat_copy_f32(&temp_2x2a, &P); // copy result back in P
}

void filter_step(uint32_t delta)
{
	static float filtered_delta = 0.0f;
	filter_predict();

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
float filter_get_frequency_offset_Hz() {
	return X.pData[0];
}

float filter_get_frequency_drift_HzDs() {
	return X.pData[1];
}
