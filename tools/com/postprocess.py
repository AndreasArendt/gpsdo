import csv
import re
import matplotlib.pyplot as plt
import kalman

#filename = r"logs/20251110_213208.csv"
#filename = r"logs/20251110_214145.csv"
#filename = r"logs/20251111_210441.csv"
filename = r"logs/20251111_212216.csv"

phase = []
freq_offset = []
freq_drift = []

try:
    kf = kalman.KalmanFilter()
    with open(filename) as file:
        reader = csv.DictReader(file)

        for row in reader:                        
            measurement = float(row["raw_counter_value"])
            
            filtered = kf.update(measurement)       
            phase.append(filtered["phase"])
            freq_offset.append(filtered["freq_offset"])
            freq_drift.append(filtered["freq_drift"])
            
except KeyboardInterrupt:
    print("stopping read loop by user")

plt.subplot(3, 1, 1)
plt.plot(phase)
plt.ylabel('phase')

plt.subplot(3, 1, 2)
plt.plot(freq_offset)
plt.ylabel('freq_offset')

plt.subplot(3, 1, 3)
plt.plot(freq_drift)
plt.ylabel('freq_drift')

plt.show()
