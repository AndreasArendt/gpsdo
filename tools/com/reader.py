import struct
import time
from kalman import KalmanFilter
from schemas.gpsdo.Status import Status
from realtime_plot import RealtimePlotter

FLATBUF_MAGIC = 0xB00B
MSG_ID_STATUS = 1
MAX_MESSAGE_SIZE = 1024

def read_loop(ser, log):
    kf = KalmanFilter()
    start_time = time.time()
    plotter = RealtimePlotter()

    log.info("Waiting for FlatBuffer messages...")

    header_fmt = "<HHH"  # little-endian: magic, msg_id, len
    header_size = struct.calcsize(header_fmt)

    while True:
        # --- Read header ---
        header = ser.read(header_size)
        if len(header) < header_size:
            continue

        magic, msg_id, msg_len = struct.unpack(header_fmt, header)

        # --- Validate magic ---
        if magic != FLATBUF_MAGIC:
            log.warning(f"Bad magic 0x{magic:04X}, searching for next valid frame...")
            magic = resync(ser, log)
            if magic != FLATBUF_MAGIC:
                continue
            # read remaining header bytes
            rest_header = ser.read(header_size - 2)
            if len(rest_header) < (header_size - 2):
                continue
            header = struct.pack("<H", magic) + rest_header
            magic, msg_id, msg_len = struct.unpack(header_fmt, header)

        # --- Validate message length ---
        if msg_len <= 0 or msg_len > MAX_MESSAGE_SIZE:
            log.warning(f"Invalid message length {msg_len}, skipping")
            continue

        # --- Read payload ---
        payload = ser.read(msg_len)
        if len(payload) < msg_len:
            log.warning("Incomplete message, skipping")
            continue

        # --- Dispatch ---
        if msg_id == MSG_ID_STATUS:
            try:
                status = Status.GetRootAsStatus(payload, 0)
                _ = status.FreqErrorHz()  # validate

                log.info(f"Freq Error (Hz): {status.FreqErrorHz():.6f}")
                log.info(f"Freq Drift (Hz/s): {status.FreqDriftHzS():.6f}")
                log.info(f"Voltage Control (V): {status.VoltageControlV():.3f}")
                log.info(f"Voltage Measured (V): {status.VoltageMeasuredV():.3f}")
                log.info(f"Temperature (C): {status.TemperatureC():.2f}")
                log.info(f"Raw Counter: {status.RawCounterValue()}")

                timestamp = time.time() - start_time
                filtered = kf.update(status.RawCounterValue())

                freq_offset_post = filtered["freq_offset_Hz"]
                freq_drift_post = filtered["drift_Hz_per_s"]                

                plotter.update(timestamp, freq_offset_post, freq_drift_post, status.FreqErrorHz(), status.FreqDriftHzS(), status.VoltageControlV(), status.VoltageMeasuredV())
            except Exception as e:                
                log.warning(f"Failed to parse Status message: {e}")
                continue
        else:
            log.info(f"Unknown msg_id={msg_id}, skipping {msg_len} bytes")
            continue


def resync(ser, log):
    """Scan stream for next magic word (2-byte sliding window)"""
    window = bytearray(2)
    idx = 0
    log.debug("Resyncing...")
    while True:
        b = ser.read(1)
        if not b:
            continue
        window[idx] = b[0]
        idx = 1 - idx  # toggle index to maintain sliding 2-byte window
        # interpret as little-endian
        val = window[0] | (window[1] << 8)
        if val == FLATBUF_MAGIC:
            log.info("Resynced to magic word")
            return val
