import logging
import argparse
from typing import Optional

from .serial_utils import open_serial_for_vid
from .gpsdo_parser import parse_delta_from_line
from .kalman import KalmanFilter

log = logging.getLogger(__name__)


def read_loop(ser, callback=None):
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
            if callback:
                callback(meas, filtered)
            else:
                print("meas:", meas, "filtered:", filtered)
    except KeyboardInterrupt:
        log.info("stopping read loop by user")
    finally:
        try:
            ser.close()
        except Exception:
            pass


def main(argv: Optional[list] = None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--vid", required=True, help="Vendor ID (e.g. 0x0483)")
    parser.add_argument("--pid", help="Product ID (e.g. 0x5740)")
    parser.add_argument("--product", help="Product substring to match")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=1.0)
    args = parser.parse_args(argv)

    vid = int(args.vid, 0)
    pid = int(args.pid, 0) if args.pid else None

    ser = open_serial_for_vid(vid, pid=pid, product=args.product, baud=args.baud, timeout=args.timeout)

    def print_cb(meas, filt):
        print(f"measurement={meas} filtered={filt}")

    read_loop(ser, callback=print_cb)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    main()