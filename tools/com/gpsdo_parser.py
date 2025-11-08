from dataclasses import dataclass
from typing import Optional
import re

@dataclass
class GpsdoData:
    delta: Optional[float] = None
    voltage_set: Optional[float] = None
    voltage_measured: Optional[float] = None

class GpsdoParser:
    _patterns = {
        'delta': re.compile(r"delta\s*:\s*([+-]?\d+(?:\.\d+)?)", re.IGNORECASE),
        'volt_set': re.compile(r"volt_set\s*:\s*([+-]?\d+(?:\.\d+)?)", re.IGNORECASE),
        'volt_meas': re.compile(r"Voltage\s*:\s*([+-]?\d+(?:\.\d+)?)", re.IGNORECASE)
    }

    @staticmethod
    def _try_parse_float(pattern: re.Pattern, line: str) -> Optional[float]:
        if match := pattern.search(line.strip()):
            try:
                return float(match.group(1))
            except ValueError:
                return None
        return None

    def parse_line(self, line: str) -> GpsdoData:
        if not line:
            return GpsdoData()
        
        return GpsdoData(
            delta=self._try_parse_float(self._patterns['delta'], line),
            voltage_set=self._try_parse_float(self._patterns['volt_set'], line),
            voltage_measured=self._try_parse_float(self._patterns['volt_meas'], line)
        )