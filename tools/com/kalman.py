import numpy as np
from collections import deque

class KalmanFilter:
    def __init__(self, T=1.0, f_counter=625_000.0, N0=625_000.0,
                 M_avg=8,   # moving average window (PPS samples)
                 output_alpha=None):  # optional exp smoothing on output (0..1)
        self.T = T
        self.f_counter = float(f_counter)
        self.N0 = float(N0)
        self.M = int(M_avg)
        self.window = deque(maxlen=self.M)

        # State: [freq_offset_Hz, drift_Hz_per_s]
        self.A = np.array([[1.0, T],
                           [0.0, 1.0]])
        self.H = np.array([[1.0, 0.0]])

        # --- Tune these for more smoothing ---
        # Make Q smaller -> filter assumes slower state evolution -> smoother
        self.Q = np.diag([0.00001, 1e-9])    # ← more smoothing than prior (was [1.0,0.01])
        # Measurement quantization step in Hz (per-1s measurement)
        s = (self.f_counter / self.N0)   # Hz per counter tick; here ≈1.0
        s_total = 16.0   # your previously used step for 10 MHz example; keep if appropriate
        # If you measured s_total empirically, use that; else use s
        # Variance of uniform quantization: s_total^2 / 12
        R0 = (s_total**2) / 12.0
        # After averaging M independent samples, variance reduces by ~M
        self.R = R0 / max(1, self.M)
        
        # init
        self.x = np.array([0.0, 0.0])
        self.P = np.eye(2) * 10.0

        # optional exponential smoothing on returned frequency
        self.out_alpha = output_alpha
        self._smoothed_freq = None

    def _counts_to_deltaf(self, mean_count):
        # Convert counts -> delta frequency in Hz (deviation from nominal)
        # For common case: each tick ~1 Hz deviation over 1s gate
        delta_f = (mean_count - self.N0) * (self.f_counter / self.N0)
        return float(delta_f)

    def update(self, raw_count: float):
        # push into moving average window
        self.window.append(float(raw_count))
        mean_count = sum(self.window) / len(self.window)

        # convert averaged counts -> delta frequency (Hz)
        measurement = self._counts_to_deltaf(mean_count)

        # Kalman predict
        x_pred = self.A.dot(self.x)
        P_pred = self.A.dot(self.P).dot(self.A.T) + self.Q

        # Kalman update
        S = self.H.dot(P_pred).dot(self.H.T) + self.R
        K = P_pred.dot(self.H.T) / S  # 2x1
        y = measurement - (self.H.dot(x_pred))[0]
        self.x = x_pred + (K.flatten() * y)
        self.P = (np.eye(2) - K.dot(self.H)).dot(P_pred)

        freq = float(self.x[0])
        drift = float(self.x[1])

        # optional small exponential smoothing on output
        if self.out_alpha is not None:
            if self._smoothed_freq is None:
                self._smoothed_freq = freq
            else:
                self._smoothed_freq = (self.out_alpha * freq +
                                       (1.0 - self.out_alpha) * self._smoothed_freq)
            freq = self._smoothed_freq

        return {"freq_offset_Hz": freq, "drift_Hz_per_s": drift}
