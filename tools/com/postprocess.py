import csv
import numpy as np
import matplotlib.pyplot as plt
import kalman
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
ema_phase = []

try:
    kf = kalman.KalmanFilter()

    # determine R
    data = pd.read_csv(filename)
    sample_var, z_freq, z_detrended = kf.estimate_R_from_counts(np.array(data["raw_counter_value"].tolist())/16)
    print(f"sample_var: {sample_var}, z_freq: {z_freq}, z_detrended: {z_detrended}")

    with open(filename) as file:
        reader = csv.DictReader(file)

        for row in reader:                        
            measurement = float(row["raw_counter_value"])
            
            X, P, k, y, ema = kf.update(measurement)      
           
            phase.append(X[0])
            freq_offset.append(X[1])
            freq_drift.append(X[2])
            raw_cnt.append(measurement)
            ema_phase.append(ema)

except KeyboardInterrupt:
    print("stopping read loop by user")

plt.subplot(5, 1, 1)
plt.plot(phase)
plt.ylabel('phase')

plt.subplot(5, 1, 2)
plt.plot(freq_offset)
plt.ylabel('freq_offset')

plt.subplot(5, 1, 3)
plt.plot(freq_drift)
plt.ylabel('freq_drift')

plt.subplot(5, 1, 4)
plt.plot(raw_cnt, marker='o')
plt.ylabel('measurement (Count)')

plt.subplot(5, 1, 5)
plt.plot(ema_phase)
plt.ylabel('ema phase (Count)')

plt.show()
