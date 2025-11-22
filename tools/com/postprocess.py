import numpy as np
import matplotlib.pyplot as plt
import kalman
import pandas as pd

filename = r"logs/20251122_204958.csv"

phase = []
freq_offset = [] 
freq_drift = []
cov__f = []

def moving_average(a, n=3):
    ret = np.cumsum(a, dtype=float)
    ret[n:] = ret[n:] - ret[:-n]
    return ret[n - 1:] / float(n)

try:
    kf = kalman.KalmanFilter()

    df = pd.read_csv(filename)

    for count in df["raw_counter_value"]:                                        
        kf.predict()
        X, P, k, y = kf.update(count)
    
        cov__f.append(P[1][1])
        phase.append(X[0])
        freq_offset.append(X[1])
        freq_drift.append(X[2])        

except KeyboardInterrupt:
    print("stopping read loop by user")


raw_cnt = df["raw_counter_value"]

plt.subplot(4, 1, 1)
plt.plot(phase)
plt.ylabel('phase')
plt.plot(df["phase_cnt"])

plt.subplot(4, 1, 2)
plt.plot(freq_offset)
plt.plot( smoothed = (
    pd.DataFrame((np.array(raw_cnt) - 5_000_000.0) * 2.0)
    .ewm(alpha=0.1)
    .mean()
    .to_numpy()
)
)
plt.fill_between(
    np.arange(len(freq_offset)),
    freq_offset - 3*np.sqrt(cov__f),
    freq_offset + 3*np.sqrt(cov__f),
    alpha=0.5
)
plt.plot(df["freq_error_hz"])
plt.ylabel('freq_offset')

plt.subplot(4, 1, 3)
plt.plot(freq_drift)
plt.ylabel('freq_drift')
plt.ylabel('ema phase (Count)')

plt.subplot(4, 1, 4)
plt.plot(raw_cnt)

plt.show()
