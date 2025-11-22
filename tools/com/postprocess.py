import csv
import numpy as np
import matplotlib.pyplot as plt
import kalman

filename = r"logs/20251122_104801.csv"

phase = []
freq_offset = []
freq_drift = []
raw_cnt = []
cov__f = []

def moving_average(a, n=3):
    ret = np.cumsum(a, dtype=float)
    ret[n:] = ret[n:] - ret[:-n]
    return ret[n - 1:] / float(n)

try:
    kf = kalman.KalmanFilter()

    with open(filename) as file:
        reader = csv.DictReader(file)

        for row in reader:                        
            measurement = float(row["raw_counter_value"])
            
            X, P, k, y = kf.update(measurement)      
           
            cov__f.append(P[1][1])
            phase.append(X[0])
            freq_offset.append(X[1])
            freq_drift.append(X[2])
            raw_cnt.append(measurement)

except KeyboardInterrupt:
    print("stopping read loop by user")


plt.subplot(4, 1, 1)
plt.plot(phase)
plt.ylabel('phase')

plt.subplot(4, 1, 2)
plt.plot(freq_offset)
plt.plot(moving_average((np.array(raw_cnt)-5_000_000.0)*2.0, n=123))
plt.fill_between(
    np.arange(len(freq_offset)),
    freq_offset - 3*np.sqrt(cov__f),
    freq_offset + 3*np.sqrt(cov__f),
    alpha=0.5
)
plt.ylabel('freq_offset')

plt.subplot(4, 1, 3)
plt.plot(freq_drift)
plt.ylabel('freq_drift')
plt.ylabel('ema phase (Count)')

plt.subplot(4, 1, 4)
plt.plot(raw_cnt)

plt.show()
