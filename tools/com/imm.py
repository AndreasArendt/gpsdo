import kalman
import numpy as np

class IMMKalman:
    def __init__(self,
                 T=1.0,
                 f_osc=10_000_000.0,
                 expected_count=625000.0,
                 sample_var_hz2=0.2):

        # Measurement noise
        r_freq = sample_var_hz2
        r_phase = r_freq / (f_osc / expected_count)**2
        R = np.diag([r_phase, r_freq])

        R_fast   = np.diag([r_phase * 0.5, r_freq * 0.5])
        R_med    = np.diag([r_phase,       r_freq])
        R_slow   = np.diag([r_phase * 2.0, r_freq * 2.0])  # hold trusts model more

        # Three process noise levels
        q_fast   = 1e-3   # fast acquisition
        q_medium = 1e-4   # normal
        q_slow   = 1e-6   # holdover / smooth

        self.models = [
            kalman.KalmanFilter(T, f_osc, expected_count, q_fast,   R_fast),
            kalman.KalmanFilter(T, f_osc, expected_count, q_medium, R_med),
            kalman.KalmanFilter(T, f_osc, expected_count, q_slow,   R_slow)
        ]

        self.M = 3
        self.mu = np.ones(3) / 3  # initial model probabilities

        # Markov transition matrix (stay most of time)
        self.PI = np.array([
            [0.90, 0.07, 0.03],
            [0.07, 0.86, 0.07],
            [0.03, 0.07, 0.90]
        ])

    def update(self, raw_count):
        M = self.M

        # --- Interaction (mixing) ---
        c = self.PI.T @ self.mu
        mix_weights = (self.PI * self.mu).T / c  # w_{j->i}

        x0 = []
        P0 = []
        for i in range(M):
            # mixed initial state
            xi = sum(mix_weights[j, i] * self.models[j].x for j in range(M))
            Pi = sum(
                mix_weights[j, i] * (
                    self.models[j].P +
                    np.outer(self.models[j].x - xi, self.models[j].x - xi)
                ) for j in range(M)
            )
            x0.append(xi)
            P0.append(Pi)

        # load mixed initial conditions
        for i in range(M):
            self.models[i].x = x0[i]
            self.models[i].P = P0[i]

        # --- Model-specific predict/update ---
        likelihoods = np.zeros(M)
        outputs = []

        for i in range(M):
            x_pred, P_pred = self.models[i].predict()
            x, P, K, y = self.models[i].update(raw_count)

            # innovation likelihood
            S = self.models[i].H @ self.models[i].P @ self.models[i].H.T + self.models[i].R
            try:
                exponent = -0.5 * (y.T @ np.linalg.inv(S) @ y)
                detS = np.linalg.det(S)
                L = np.exp(exponent) / (2 * np.pi * np.sqrt(detS))
            except np.linalg.LinAlgError:
                L = 1e-12

            likelihoods[i] = max(L, 1e-12)
            outputs.append((x, P, K, y))

        # --- Update model probabilities ---
        new_mu = c * likelihoods
        new_mu /= new_mu.sum()
        self.mu = new_mu

        # --- Combined output ---
        x_comb = sum(self.mu[i] * outputs[i][0] for i in range(M))
        P_comb = sum(
            self.mu[i] * (
                outputs[i][1] +
                np.outer(outputs[i][0] - x_comb, outputs[i][0] - x_comb)
            ) for i in range(M)
        )

        # return combined KF output
        # K, y from most probable model
        best = np.argmax(self.mu)
        K, y = outputs[best][2], outputs[best][3]

        return x_comb, P_comb, K, y
