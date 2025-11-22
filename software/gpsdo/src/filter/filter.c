/*
 * filter.c
 */

#include "filter.h"
#include "gpsdo_config.h"
#include <arm_math.h>
#include <string.h>
#include <math.h>

static void filter_fill_debug(float S_val, float mahal_dist);

#define ARM_MATH_MATRIX_CHECK 1
#define MAHAL_THRESHOLD   9.0f   // 3-sigma rejection

static KF_DebugSnapshot kf_snapshot;
static uint32_t kf_outlier_count = 0;
static uint32_t kf_iteration_counter = 0;

// Sampling interval (seconds)
static const float T = 1.0f;

// Derived ratio: counts per oscillator cycle over interval T
static const float r = (float) EXPECTED_CTR / ((float) F_OSC_HZ * T);

// ==== Raw storage for matrices & vectors ====

// State transition F (3x3)
static float F_data[9] = { 1.0f, r * T, 0.5f * r * T * T, 0.0f, 1.0f, T, 0.0f,
		0.0f, 1.0f };

// Measurement matrix H (1x3): phase only
static float H_data[3] = { 1.0f, 0.0f, 0.0f };
// H^T (3x1)
static float HT_data[3] = { 1.0f, 0.0f, 0.0f };

// Identity I (3x3)
static float I_data[9] =
		{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

static float Q_data[9] = { 0.0f };  // process noise (3x3)
static float R_data[1] = { 0.0f };  // measurement noise (1x1)
static float P_data[9] = { 0.0f };  // covariance (3x3)
static float X_data[3] = { 0.0f };  // state (3x1)
static float X_pred_data[3] = { 0.0f };  // predicted state (3x1)

// For prediction step
static float FT_data[9] = { 0.0f };       // F^T (3x3)

// Temp matrices
static float temp_3x3a_data[9] = { 0.0f };
static float temp_3x3b_data[9] = { 0.0f };
static float temp_1x3_data[3] = { 0.0f };  // H P (1x3)
static float temp_1x1a_data[1] = { 0.0f };  // HPHT or inv(S) (1x1)
static float temp_3x1a_data[3] = { 0.0f };  // P HT (3x1)
static float temp_3x1b_data[3] = { 0.0f };  // K y (3x1)

// Measurement and innovation
static float z_data[1] = { 0.0f };   // z = measured phase (cycles)
static float y_data[1] = { 0.0f };   // innovation

// HX = H * X_pred
static float HX_data[1] = { 0.0f };

// Kalman gain K (3x1)
static float K_data[3] = { 0.0f };

// ==== CMSIS matrix instances ====
static arm_matrix_instance_f32 F, FT, H, HT, Q, R, I;
static arm_matrix_instance_f32 P, X, X_pred, HX;
static arm_matrix_instance_f32 temp_3x3a, temp_3x3b;
static arm_matrix_instance_f32 temp_1x3, temp_1x1a;
static arm_matrix_instance_f32 temp_3x1a, temp_3x1b;
static arm_matrix_instance_f32 z, y;
static arm_matrix_instance_f32 K;

// Simple memcpy helper
static inline void mat_copy(const arm_matrix_instance_f32 *src,
		arm_matrix_instance_f32 *dst) {
	memcpy(dst->pData, src->pData, src->numRows * src->numCols * sizeof(float));
}

void filter_init(void) {
	// ---- Process noise Q (constant-acceleration style) ----
	// Tunable scalar q: bigger -> filter responds faster, noisier
	const float q = 1e-5f;

	Q_data[0] = q * T * T * T * T * T / 20.0f;
	Q_data[1] = q * T * T * T * T / 8.0f;
	Q_data[2] = q * T * T * T / 6.0f;
	Q_data[3] = q * T * T * T * T / 8.0f;
	Q_data[4] = q * T * T * T / 3.0f;
	Q_data[5] = q * T * T / 2.0f;
	Q_data[6] = q * T * T * T / 6.0f;
	Q_data[7] = q * T * T / 2.0f;
	Q_data[8] = q * T;

	// ---- Measurement noise R ----
	// Measurement is raw_count - EXPECTED_CTR in cycles at 5 MHz.
	// For 1 second gate and ±1 count resolution, σ_phase ≈ 0.29 cycles.
	const float sigma_phase = 0.3f; // cycles (tunable)
	R_data[0] = sigma_phase * sigma_phase;  // 1x1

	// ---- Initial covariance P ----
	// Phase:   ±0.2 cycles
	// Freq:    ±2 Hz
	// Drift:   ±0.02 Hz/s
	memset(P_data, 0, sizeof(P_data));
	P_data[0] = 0.20f * 0.20f;
	P_data[4] = 2.0f * 2.0f;
	P_data[8] = 0.02f * 0.02f;

	// ---- State ----
	X_data[0] = 0.0f; // phase
	X_data[1] = 0.0f; // frequency offset (Hz)
	X_data[2] = 0.0f; // drift (Hz/s)

	memcpy(X_pred_data, X_data, sizeof(X_data));

	// ---- Matrix instances ----
	arm_mat_init_f32(&F, 3, 3, F_data);
	arm_mat_init_f32(&FT, 3, 3, FT_data);
	arm_mat_init_f32(&H, 1, 3, H_data);
	arm_mat_init_f32(&HT, 3, 1, HT_data);
	arm_mat_init_f32(&Q, 3, 3, Q_data);
	arm_mat_init_f32(&R, 1, 1, R_data);
	arm_mat_init_f32(&I, 3, 3, I_data);

	arm_mat_init_f32(&P, 3, 3, P_data);
	arm_mat_init_f32(&X, 3, 1, X_data);
	arm_mat_init_f32(&X_pred, 3, 1, X_pred_data);

	arm_mat_init_f32(&HX, 1, 1, HX_data);
	arm_mat_init_f32(&z, 1, 1, z_data);
	arm_mat_init_f32(&y, 1, 1, y_data);

	arm_mat_init_f32(&temp_3x3a, 3, 3, temp_3x3a_data);
	arm_mat_init_f32(&temp_3x3b, 3, 3, temp_3x3b_data);
	arm_mat_init_f32(&temp_1x3, 1, 3, temp_1x3_data);
	arm_mat_init_f32(&temp_1x1a, 1, 1, temp_1x1a_data);
	arm_mat_init_f32(&temp_3x1a, 3, 1, temp_3x1a_data);
	arm_mat_init_f32(&temp_3x1b, 3, 1, temp_3x1b_data);

	arm_mat_init_f32(&K, 3, 1, K_data);

	// Precompute F^T
	arm_mat_trans_f32(&F, &FT);
}

void filter_predict(float v) {
	// X_pred = F * X
	arm_mat_mult_f32(&F, &X, &X_pred);

	// P = F P F^T + Q
	arm_mat_mult_f32(&F, &P, &temp_3x3a);  // temp_3x3a = FP
	arm_mat_mult_f32(&temp_3x3a, &FT, &temp_3x3b); // temp_3x3b = FPF^T
	arm_mat_add_f32(&temp_3x3b, &Q, &P);     // P = FPF^T + Q
}

void filter_correct(float raw_count) {
	float z_phase = raw_count - (float) EXPECTED_CTR;
	z_data[0] = z_phase;

	// y = z - H * X_pred
	arm_mat_mult_f32(&H, &X_pred, &HX);  // HX = H X_pred
	arm_mat_sub_f32(&z, &HX, &y);        // y = z - HX

	// S = H P H^T + R
	arm_mat_mult_f32(&H, &P, &temp_1x3);   // temp_1x3 = HP
	arm_mat_mult_f32(&temp_1x3, &HT, &temp_1x1a); // temp_1x1a = HPH^T

	// temp_1x1a = S = HPHT + R
	temp_1x1a_data[0] += R_data[0];

	// invS = 1 / S
	float S_val = temp_1x1a_data[0];
	float innov = y_data[0];

	// Compute Mahalanobis D^2 = y^2 / S
	float mahal_dist = (innov * innov) / S_val;

	// Outlier detected — skip correction
	if (mahal_dist > MAHAL_THRESHOLD) {
		kf_outlier_count++;
	    mat_copy(&X_pred, &X);

	    filter_fill_debug(S_val, mahal_dist);
	    return;
	}

	float invS = 1.0f / S_val;
	temp_1x1a_data[0] = invS;   // reuse temp_1x1a as inv(S)

	// K = P H^T inv(S)
	arm_mat_mult_f32(&P, &HT, &temp_3x1a);        // temp_3x1a = P H^T
	arm_mat_mult_f32(&temp_3x1a, &temp_1x1a, &K); // K = P H^T * inv(S)

	// X = X_pred + K y
	arm_mat_mult_f32(&K, &y, &temp_3x1b);  // temp_3x1b = K y
	arm_mat_add_f32(&X_pred, &temp_3x1b, &X);

	// P = (I - K H) P
	arm_mat_mult_f32(&K, &H, &temp_3x3a);         // temp_3x3a = K H
	arm_mat_sub_f32(&I, &temp_3x3a, &temp_3x3b);  // temp_3x3b = I - K H
	arm_mat_mult_f32(&temp_3x3b, &P, &temp_3x3a); // temp_3x3a = (I - K H) P

	// Copy back to P
	mat_copy(&temp_3x3a, &P);

    filter_fill_debug(S_val, mahal_dist);
}

void filter_step(float raw, float v) {
	filter_predict(v);
	filter_correct(raw);

	kf_iteration_counter++;
}


static void filter_fill_debug(float S_val, float mahal_dist) {
    kf_snapshot.timestamp_s = 0.0f; //get_timestamp_seconds();

    kf_snapshot.x[0] = X_data[0];
    kf_snapshot.x[1] = X_data[1];
    kf_snapshot.x[2] = X_data[2];
    memcpy(kf_snapshot.P, P_data, sizeof(float)*9);

    kf_snapshot.z  = z_data[0];
    kf_snapshot.h_x = HX_data[0];
    kf_snapshot.y = y_data[0];
    kf_snapshot.S = S_val;
    kf_snapshot.mahal_d2 = mahal_dist;
    kf_snapshot.nis = mahal_dist;
    kf_snapshot.rejected = (mahal_dist > MAHAL_THRESHOLD);

    memcpy(kf_snapshot.K, K_data, sizeof(float)*3);
    memcpy(kf_snapshot.H, H_data, sizeof(float)*3);
    memcpy(kf_snapshot.Q, Q_data, sizeof(float)*9);
    kf_snapshot.R = R_data[0];

    kf_snapshot.outlier_count = kf_outlier_count;
    kf_snapshot.iteration = kf_iteration_counter;
}

// ------------ GETTERS ------------
float filter_get_phase_count(void) {
	return X_data[0];
}

float filter_get_frequency_offset_Hz(void) {
	return X_data[1];
}

float filter_get_frequency_drift_HzDs(void) {
	return X_data[2];
}

void filter_get_kf_debug_flatbuf(KF_DebugSnapshot *dst)
{
    memcpy(dst, &kf_snapshot, sizeof(KF_DebugSnapshot));
}
