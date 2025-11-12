import numpy as np

class KalmanFilter:
    def __init__(self, T_PPS_s=1.0, f_osc_Hz=10_000_000.0, expected_counter=625_000.0):
        self.T = T_PPS_s
        self.f_osc_Hz = float(f_osc_Hz)
        self.expected_counter = float(expected_counter)
        self.ema_prev = None
        self.total_phase_error = 0.0

        # State: [phase_cycles, freq_offset_Hz, drift_Hz_per_s]
        self.A = np.array([
            [1.0, self.T, 0.5 * self.T**2],
            [0.0, 1.0, self.T],
            [0.0, 0.0, 1.0]
        ])

        # One measurement: phase
        self.H = np.array([[1.0, 0.0, 0.0]])  # 1x3

        q = 1e-7 # Hz2/s3 (white noise spectral density)
        self.Q = q * np.array([
            [self.T**5/20, self.T**4/8, self.T**3/6],
            [self.T**4/8,  self.T**3/3, self.T**2/2],
            [self.T**3/6,  self.T**2/2,  self.T]
        ])

        s = self.f_osc_Hz / self.expected_counter
        self.R = np.array([[ (s**2) / 12.0 ]])  # 1x1

        self.x = np.zeros(3)
        self.P = np.eye(3) * 10.0

    def _counts_to_deltaf(self, mean_count):
        return (mean_count - self.expected_counter) * (self.f_osc_Hz / self.expected_counter)

    def filter_ema(self, val, alpha=0.3):
        if self.ema_prev is None:
            self.ema_prev = val
        self.ema_prev = alpha * val + (1 - alpha) * self.ema_prev
        return self.ema_prev

    def update(self, raw_count):
        count_filt = self.filter_ema(raw_count)
        delta_f = self._counts_to_deltaf(count_filt)
        self.total_phase_error += delta_f * self.T  # integrate frequency to get phase

        z = np.array([self.total_phase_error])  # measurement: phase

        # Prediction
        x_pred = self.A @ self.x
        P_pred = self.A @ self.P @ self.A.T + self.Q

        # Update
        S = self.H @ P_pred @ self.H.T + self.R
        K = P_pred @ self.H.T / S
        y = z - self.H @ x_pred

        self.x = x_pred + (K.flatten() * y)
        I = np.eye(3)
        self.P = (I - K @ self.H) @ P_pred

        phase = float(self.x[0])
        freq_offset = float(self.x[1])
        freq_drift = float(self.x[2])

        return {"phase": phase, 
                "freq_offset": freq_offset,
                "freq_drift": freq_drift}
