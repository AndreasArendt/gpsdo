import gpsdo_parser
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
            meas = gpsdo_parser.parse_delta_from_line(line)
            if meas is None:
                volt_set = gpsdo_parser.parse_volt_set_from_line(line)
                
                if volt_set is None:
                    volt_meas = gpsdo_parser.parse_volt_meas_from_line(line)
                    if volt_meas is not None:
                        log.info("volt_meas: %s", volt_meas)                        
                    continue
                else:
                    log.info("volt_set: %s", volt_set)
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