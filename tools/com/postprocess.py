import os
import sys
import re

try:
    from .kalman import KalmanFilter
except ImportError:
    # When run directly inside gpsdo/tools/com/
    pkg_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    if pkg_root not in sys.path:
        sys.path.insert(0, pkg_root)
    from gpsdo.tools.com.kalman import KalmanFilter    

filename = r"251028.log"

try:
    kf = KalmanFilter()
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
            print(f"measurement={meas} filtered={filtered}")
except KeyboardInterrupt:
    print("stopping read loop by user")