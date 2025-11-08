import json
import re
from typing import Optional

_delta_re = re.compile(r"delta\s*:\s*([+-]?\d+(?:\.\d+)?)", re.IGNORECASE)
_volt_set_re = re.compile(r"volt_set\s*:\s*([+-]?\d+(?:\.\d+)?)", re.IGNORECASE)
_volt_meas_re = re.compile(r"Voltage\s*:\s*([+-]?\d+(?:\.\d+)?)", re.IGNORECASE)

def parse_volt_meas_from_line(line: str) -> Optional[float]:
    if not line:
        return None
    line = line.strip()
    m = _volt_meas_re.search(line)
    if m:
        try:
            return float(m.group(1))
        except ValueError:
            return None

def parse_volt_set_from_line(line: str) -> Optional[float]:
    if not line:
        return None
    line = line.strip()
    m = _volt_set_re.search(line)
    if m:
        try:
            return float(m.group(1))
        except ValueError:
            return None

def parse_delta_from_line(line: str) -> Optional[float]:
    if not line:
        return None
    line = line.strip()
    m = _delta_re.search(line)
    if m:
        try:
            return float(m.group(1))
        except ValueError:
            return None
    try:
        obj = json.loads(line)
        if isinstance(obj, dict) and "delta" in obj:
            val = obj["delta"]
            if isinstance(val, (int, float)):
                return float(val)
    except json.JSONDecodeError:
        pass
    return None