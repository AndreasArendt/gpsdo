import numpy as np
import matplotlib.pyplot as plt
import kalman
import pandas as pd

filename = r"logs/20251123_102614.csv"

phase = []
freq_offset = [] 
freq_drift = []
cov = np.empty((0, 3))
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
            
        cov = np.vstack([cov, np.diagonal(P, axis1=0, axis2=1).T])        
        phase.append(X[0])
        freq_offset.append(X[1])
        freq_drift.append(X[2])        

except KeyboardInterrupt:
    print("stopping read loop by user")

#############################################
plt.subplot(4, 1, 1)
plt.plot(phase, label="Phase")
plt.fill_between(
    np.arange(len(phase)),
    phase - 3*np.sqrt(cov[:,0]),
    phase + 3*np.sqrt(cov[:,0]),
    alpha=0.5,
    label="3-sigma"
)
plt.plot(df["phase_cnt"], label="Phase (log)")
plt.ylabel('phase')
plt.legend()

#############################################
plt.subplot(4, 1, 2)
plt.plot(freq_offset, label="Frequency Offset")
plt.plot( smoothed = (
    pd.DataFrame((np.array(df["raw_counter_value"]) - 5_000_000.0) * 2.0)
    .ewm(alpha=0.1)
    .mean()
    .to_numpy()
), label="Frequency Offset (raw, smoothed)" )
plt.fill_between(
    np.arange(len(freq_offset)),
    freq_offset - 3*np.sqrt(cov[:,1]),
    freq_offset + 3*np.sqrt(cov[:,1]),
    alpha=0.5,
    label="3-sigma"
)
plt.plot(df["freq_error_hz"], label="Frequency Offset (log)")
plt.ylabel('freq_offset')
plt.legend()

#############################################
plt.subplot(4, 1, 3)
plt.plot(freq_drift, label="Frequency Drift")
plt.fill_between(
    np.arange(len(freq_drift)),
    freq_drift - 3*np.sqrt(cov[:,1]),
    freq_drift + 3*np.sqrt(cov[:,1]),
    alpha=0.5,
    label="3-sigma"
)
plt.plot(df["freq_drift_hz_s"], label="Frequency Drift (log)")
plt.ylabel('freq_drift')
plt.ylabel('ema phase (Count)')
plt.legend()

#############################################
plt.subplot(4, 1, 4)
plt.plot(df["raw_counter_value"], label="Raw Counts")
#plt.plot(df["voltage_control_v"], label="V control")
#plt.plot(df["voltage_measured_v"], label="V meas")
plt.legend()

plt.show()
