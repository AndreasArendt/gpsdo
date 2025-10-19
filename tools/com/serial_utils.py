import logging
from typing import Optional

import serial
from serial.tools import list_ports

log = logging.getLogger(__name__)


def find_serial_port_by_vid_product(vid: int, pid: Optional[int] = None, product: Optional[str] = None) -> Optional[str]:
    for p in list_ports.comports():
        if p.vid is None or p.vid != vid:
            continue
        if pid is not None and (p.pid is None or p.pid != pid):
            continue
        if product is not None and product not in (p.product or ""):
            continue
        return p.device
    return None


def open_serial_for_vid(vid: int, pid: Optional[int] = None, product: Optional[str] = None,
                        baud: int = 115200, timeout: float = 1.0) -> serial.Serial:
    device = find_serial_port_by_vid_product(vid, pid=pid, product=product)
    if not device:
        pid_info = f" pid={pid:#x}" if pid is not None else ""
        prod_info = f" product='{product}'" if product is not None else ""
        raise RuntimeError(f"serial device with vid={vid:#x}{pid_info}{prod_info} not found")
    ser = serial.Serial(device, baudrate=baud, timeout=timeout)
    log.info("Opened serial %s @ %d", device, baud)
    return ser