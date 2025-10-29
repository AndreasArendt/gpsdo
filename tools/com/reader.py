from .gpsdo_parser import parse_delta_from_line
from .kalman import KalmanFilter

def read_loop(ser, log):
    try:
        kf = KalmanFilter()
        while True:
            raw = ser.readline()
            if not raw:
                continue
            try:
                line = raw.decode("utf-8", errors="replace")
            except Exception:
                line = raw.decode("latin-1", errors="replace")
            log.debug("recv: %s", line.strip())
            meas = parse_delta_from_line(line)
            if meas is None:
                log.debug("no measurement parsed")
                continue

            filtered = kf.update(meas)            
            log.info(f"measurement={meas} filtered={filtered}")
    except KeyboardInterrupt:
        log.info("stopping read loop by user")
    finally:
        try:
            ser.close()
        except Exception:
            pass