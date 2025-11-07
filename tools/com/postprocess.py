import os
import sys
import re
import matplotlib.pyplot as plt

try:
    from .kalman import KalmanFilter
except ImportError:
    # When run directly inside gpsdo/tools/com/
    pkg_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    if pkg_root not in sys.path:
        sys.path.insert(0, pkg_root)
    from gpsdo.tools.com.kalman import KalmanFilter    

filename = r"reader1.log"

freq_offset_Hz = []
drift_Hz_per_s = []

try:
    kf = KalmanFilter(output_alpha=0.2)
    with open(filename) as file:
        while line := file.readline():
            _delta_re = re.compile(r"measurement=(\d+(?:\.\d+)?)", re.IGNORECASE)
            m = _delta_re.search(line.strip())

            measurement = None
            if m:
                try:
                    meas = float(m.group(1))
                except ValueError:
                    continue
            else:
                continue

            filtered = kf.update(meas)       
            freq_offset_Hz.append(filtered["freq_offset_Hz"])
            drift_Hz_per_s.append(filtered["drift_Hz_per_s"])

            #print(f"measurement={meas} filtered={filtered}")
except KeyboardInterrupt:
    print("stopping read loop by user")

plt.subplot(2, 1, 1)
plt.plot(freq_offset_Hz)
plt.ylabel('freq_offset_Hz')

plt.subplot(2, 1, 2)
plt.plot(drift_Hz_per_s)
plt.ylabel('drift_Hz_per_s')
plt.show()