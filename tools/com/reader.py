import struct
import time
import csv
import os
from datetime import datetime
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

    # Setup CSV logging
    csv_dir = "logs"
    if not os.path.exists(csv_dir):
        os.makedirs(csv_dir)
    
    timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = os.path.join(csv_dir, f"{timestamp_str}.csv")
    
    csv_file = open(csv_filename, 'w', newline='')
    csv_writer = csv.writer(csv_file)
    # Write header
    csv_writer.writerow([
        'timestamp_s',
        'freq_error_hz',
        'freq_drift_hz_s',
        'voltage_control_v',
        'voltage_measured_v',
        'temperature_c',
        'raw_counter_value',
        'filtered_freq_offset_hz',
        'filtered_drift_hz_s'
    ])
    csv_file.flush()

    log.info(f"Waiting for FlatBuffer messages... CSV logging to {csv_filename}")

    header_fmt = "<HHH"  # little-endian: magic, msg_id, len
    header_size = struct.calcsize(header_fmt)

    try:
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

                    freq_error_hz = status.FreqErrorHz()
                    freq_drift_hz_s = status.FreqDriftHzS()
                    voltage_control_v = status.VoltageControlV()
                    voltage_measured_v = status.VoltageMeasuredV()
                    temperature_c = status.TemperatureC()
                    raw_counter_value = status.RawCounterValue()

                    log.info(f"Freq Error (Hz): {freq_error_hz:.6f}")
                    log.info(f"Freq Drift (Hz/s): {freq_drift_hz_s:.6f}")
                    log.info(f"Voltage Control (V): {voltage_control_v:.3f}")
                    log.info(f"Voltage Measured (V): {voltage_measured_v:.3f}")
                    log.info(f"Temperature (C): {temperature_c:.2f}")
                    log.info(f"Raw Counter: {raw_counter_value}")

                    timestamp = time.time() - start_time
                    filtered = kf.update(raw_counter_value)

                    freq_offset_post = filtered["freq_offset_Hz"]
                    freq_drift_post = filtered["drift_Hz_per_s"]

                    # Log to CSV
                    csv_writer.writerow([
                        timestamp,
                        freq_error_hz,
                        freq_drift_hz_s,
                        voltage_control_v,
                        voltage_measured_v,
                        temperature_c,
                        raw_counter_value,
                        freq_offset_post,
                        freq_drift_post
                    ])
                    csv_file.flush()

                    plotter.update(timestamp, freq_offset_post, freq_drift_post, freq_error_hz, freq_drift_hz_s, voltage_control_v, voltage_measured_v)
                except Exception as e:
                    log.warning(f"Failed to parse Status message: {e}")
                    continue
            else:
                log.info(f"Unknown msg_id={msg_id}, skipping {msg_len} bytes")
                continue
    finally:
        csv_file.close()
        log.info(f"CSV file closed: {csv_filename}")


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
