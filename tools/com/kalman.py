import numpy as np

class KalmanFilter:
    """
    3-state phase / frequency / drift Kalman Filter optimized for:
        - 10 MHz → 5 MHz divided GPSDO sampling
        - fast initial acquisition
        - long-term stability for frequency control
    """

    def __init__(self, 
                 T=1.0, 
                 f_osc=10_000_000.0, 
                 expected_count=5_000_000.0, 
                 q=1e-5,
                 sigma_phase = 0.5/np.sqrt(3)):
        self.T = T
        self.f_osc = f_osc
        self.expected_count = expected_count

        r = self.expected_count / (self.f_osc * self.T)
        
        # State Transition matrix
        self.A = np.array([
            [1.0,   r*T,      0.5*r*T**2],
            [0.0,   1.0,      T],
            [0.0,   0.0,      1.0]
        ])

        # Measurement Model
        self.H = np.array([[1.0, 0.0, 0.0]])

        # Process noise
        Q_unit = np.array([
            [T**5/20, T**4/8, T**3/6],
            [T**4/8,  T**3/3, T**2/2],
            [T**3/6,  T**2/2, T]
        ])
        self.Q = q * Q_unit
        
        # Measurement noise              
        self.R = np.array([[sigma_phase**2]])
                
        # Initial State & Covariance        
        self.x = np.zeros(3)

        self.P = np.diag([
            (0.20)**2,    # ±0.2 cycles initial phase uncertainty
            (2.0)**2,     # ±2 Hz     initial freq
            (0.02)**2     # ±0.02 Hz/s drift (tight but realistic)
        ])

    def predict(self):
        self.x = self.A @ self.x
        self.P = self.A @ self.P @ self.A.T + self.Q
        return self.x, self.P

    def update(self, raw_count):
        # Innovation & Kalman Gain
        S = self.H @ self.P @ self.H.T + self.R
        K = self.P @ self.H.T @ np.linalg.inv(S)

        z = raw_count - self.expected_count
        y = z - (self.H @ self.x)

        # State update
        self.x = self.x + K @ y

        # Joseph form
        I = np.eye(3)
        I_KH = I - K @ self.H
        self.P = I_KH @ self.P @ I_KH.T + K @ self.R @ K.T

        return self.x, self.P, K, y