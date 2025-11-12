import csv
import re
import matplotlib.pyplot as plt
import kalman

filename = r"logs/20251110_213208.csv"

freq_offset_Hz = []
drift_Hz_per_s = []

try:
    kf = kalman.KalmanFilter(output_alpha=0.2)
    with open(filename) as file:
        reader = csv.DictReader(file)

        for row in reader:                        
            measurement = row["raw_counter_value"]
            
            filtered = kf.update(measurement)       
            freq_offset_Hz.append(filtered["freq_offset_Hz"])
            drift_Hz_per_s.append(filtered["drift_Hz_per_s"])
            
except KeyboardInterrupt:
    print("stopping read loop by user")

plt.subplot(2, 1, 1)
plt.plot(freq_offset_Hz)
plt.ylabel('freq_offset_Hz')

plt.subplot(2, 1, 2)
plt.plot(drift_Hz_per_s)
plt.ylabel('drift_Hz_per_s')
plt.show()