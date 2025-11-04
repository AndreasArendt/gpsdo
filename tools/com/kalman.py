import numpy as np
from collections import deque

class KalmanFilter:
    def __init__(self
                 , T_PPS_Hz=1.0 # time step in seconds (PPS interval)
                 , f_osc_Hz=10_000_000.0 # oscillator frequency in Hz
                 , expected_counter=625_000.0 # nominal counter value over T
                 , window_size=8 # moving average window (PPS samples)
                 , output_alpha=None):  # exp output smoothing [0..1]
        self.T_PPS_Hz = T_PPS_Hz
        self.f_osc_Hz = float(f_osc_Hz)
        self.expected_counter = float(expected_counter)
        self.window_size = int(window_size)
        self.window = deque(maxlen=self.window_size)

        # State: [freq_offset_Hz, drift_Hz_per_s]
        self.A = np.array([[1.0, T_PPS_Hz],
                           [0.0, 1.0]])
        self.H = np.array([[1.0, 0.0]])
        
        self.Q = np.diag([0.00001, 1e-9])
        
        # Calculate measurement covariance R
        s = self.f_osc_Hz / self.expected_counter        
        R0 = (s**2) / 12.0 # variance of uniform dist over 1 count
        self.R = R0 / max(1, self.window_size)
        
        # init
        self.x = np.array([0.0, 0.0])
        self.P = np.eye(2) * 10.0

        # optional exponential smoothing on returned frequency
        self.out_alpha = output_alpha
        self._smoothed_freq = None

    def _counts_to_deltaf(self, mean_count):        
        delta_f = (mean_count - self.expected_counter) * (self.f_osc_Hz / self.expected_counter)
        return float(delta_f)

    def update(self, raw_count: float):
        # moving average window
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

        maha = (y*y) / S
        if maha > 9.21:  # chi2 1 DOF ~ 99%
            # ignore update or inflate R for this step
            print("KalmanFilter: Mahalanobis distance too high, inflating R")
            self.R *= 10.0
        
        self.x = x_pred + (K.flatten() * y)
        I = np.eye(2)
        self.P = (I - K.dot(self.H)).dot(P_pred).dot((I - K.dot(self.H)).T) + K.dot(self.R).dot(K.T)
        
        freq = float(self.x[0])
        drift = float(self.x[1])

        # optional small exponential smoothing on output
        if self.out_alpha is not None:
            if self._smoothed_freq is None:
                self._smoothed_freq = freq
            else:
                self._smoothed_freq = (self.out_alpha * freq + (1.0 - self.out_alpha) * self._smoothed_freq)
            freq = self._smoothed_freq

        return {"freq_offset_Hz": freq, "drift_Hz_per_s": drift}
