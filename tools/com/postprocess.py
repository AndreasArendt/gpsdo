import csv
import numpy as np
import matplotlib.pyplot as plt
import imm
import pandas as pd

#filename = r"logs/20251110_213208.csv"
#filename = r"logs/20251110_214145.csv"
#filename = r"logs/20251111_210441.csv"
filename = r"logs/20251111_212216.csv"
#filename = r"logs/20251113_214011.csv"

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
    kf = imm.IMMKalman()

    # determine R
    # data = pd.read_csv(filename)
    # sample_var, z_freq, z_detrended = kf.estimate_R_from_counts(np.array(data["raw_counter_value"].tolist())/16)
    # print(f"sample_var: {sample_var}, z_freq: {z_freq}, z_detrended: {z_detrended}")

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

plt.subplot(3, 1, 1)
plt.plot(phase)
plt.ylabel('phase')

plt.subplot(3, 1, 2)
plt.plot(freq_offset)
plt.plot(moving_average((np.array(raw_cnt)-625_000.0)*16.0, n=123), marker='o')
plt.plot(np.sqrt(cov__f))
plt.ylabel('freq_offset')

plt.subplot(3, 1, 3)
plt.plot(freq_drift)
plt.ylabel('freq_drift')
plt.ylabel('ema phase (Count)')

plt.show()
