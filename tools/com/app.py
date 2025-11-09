import os
import sys
import argparse
from typing import Optional
import logging

log = logging.getLogger(__name__)

from serial_utils import open_serial_for_vid
from reader import read_loop

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

    read_loop(ser, log)

if __name__ == "__main__":
    # Setup logging — log to both file and console
    log_file = os.path.join(os.path.dirname(__file__), "reader.log")
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s,%(msecs)03d %(name)s %(levelname)s %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        handlers=[
            logging.FileHandler(log_file, mode="w"),
            logging.StreamHandler(sys.stdout),
        ],
    )

    # Default VID/PID if none given
    VID = "0x0483"
    PID = "0x5740"
    argv = ["--vid", VID, "--pid", PID]
    main(argv)