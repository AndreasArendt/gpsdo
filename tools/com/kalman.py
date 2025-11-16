import numpy as np
import matplotlib.pyplot as plt

class KalmanFilter:
    def __init__(self, T=1.0, f_osc=10_000_000.0, expected_count=625_000.0,
                 q=1e-6, sample_var_hz2=0.08, ema_alpha=0.175):
        self.T = float(T)
        self.f_osc = float(f_osc)
        self.expected_count = float(expected_count)   # expected counts per PPS gate
        self. ema_alpha = float(ema_alpha)

        self.ema_phase = 0.0

        # State: [phase_cycles, freq_Hz, drift_Hz_per_s]
        self.A = np.array([
            [1.0, self.T, 0.5*self.T**2],
            [0.0, 1.0,     self.T      ],
            [0.0, 0.0,     1.0        ]
        ])

        # 2x3 H (phase, freq)
        self.H = np.array([[1.0, 0.0, 0.0],   # phase in cycles
                           [0.0, 1.0, 0.0]])  # freq in Hz

        # Process noise - q is spectral density of freq drift
        self.q = float(q)
        self.Q = self.q * np.array([
            [self.T**5/20, self.T**4/8, self.T**3/6],
            [self.T**4/8,  self.T**3/3, self.T**2/2],
            [self.T**3/6,  self.T**2/2, self.T     ]
        ])

        # Measurement noise: Phase noise [cycles^2], Freq Noise in [Hz^2]                            
        r_freq_hz2 = sample_var_hz2             
        r_phase_cycles2 = sample_var_hz2 / (f_osc/expected_count)**2
      
        self.R = np.diag([float(r_phase_cycles2), float(r_freq_hz2)])        

        # initial state and covariance
        self.x = np.array([0.0, 0.0, 0.0])
        self.P = np.diag([0.5**2, 5.0**2, 0.1**2])

        self.prev_count = None

    def _measurements(self, raw_count):
        phase_error = raw_count - self.expected_count
        self.ema_phase = self.ema_alpha * phase_error + (1 - self.ema_alpha) * self.ema_phase
        
        # Phase in cycles
        z_phase = self.ema_phase

        # Frequency in Hz: (phase error / expected count) * f_osc
        z_freq_hz = z_phase / self.expected_count * self.f_osc

        return z_phase, z_freq_hz

    def update(self, raw_count):
        z_phase, z_freq_hz = self._measurements(raw_count)
        z = np.array([z_phase, z_freq_hz])

        # Predict
        x_pred = self.A @ self.x
        P_pred = self.A @ self.P @ self.A.T + self.Q

        # Kalman update
        S = self.H @ P_pred @ self.H.T + self.R
        K = P_pred @ self.H.T @ np.linalg.inv(S)
        
        y = z - (self.H @ x_pred)
        
        # wrap phase innovation if necessary
        if y[0] > 0.5:
            y[0] -= 1.0
        elif y[0] < -0.5:
            y[0] += 1.0

        self.x = x_pred + (K @ y)
        IminusKH = (np.eye(3) - K @ self.H) @ P_pred
        self.P = IminusKH @ P_pred @ IminusKH.T + K @ self.R @ K.T

        # keep phase fractional
        int_cycles = np.round(self.x[0])
        self.x[0] -= int_cycles

        return self.x.copy(), self.P.copy(), K, y, self.ema_phase
    
    def estimate_R_from_counts(self, raw_counts, f_osc=10e6, N=625000.0):
        # convert to instantaneous measured frequency (Hz)
        z_freq = (raw_counts - N) / N * f_osc   # array in Hz
        # remove slow trend (detrend) because R should reflect measurement noise not slow drift
        z_detrended = z_freq - np.polyval(np.polyfit(np.arange(len(z_freq)), z_freq, 1), np.arange(len(z_freq)))
        sample_var = np.var(z_detrended, ddof=1)
        return sample_var, z_freq, z_detrended
