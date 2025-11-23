import time
import csv
import os
import numpy as np
from datetime import datetime
from kalman import KalmanFilter
from flatbuffer_reader import FlatbufferStreamReader
from schemas.gpsdo.Payload import Payload
from realtime_plot import RealtimePlotter

def read_loop(ser, log):    
    kf = KalmanFilter(1.0, 10_000_000.0, 5_000_000.0, 1e-6, 0.08)
    start_time = time.time()
    plotter = RealtimePlotter()
    fb_reader = FlatbufferStreamReader(ser, log)

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
        'phase_cnt',
        'freq_error_hz',
        'freq_drift_hz_s',
        'voltage_control_v',
        'voltage_measured_v',
        'temperature_c',
        'raw_counter_value'        
    ])
    csv_file.flush()

    log.info(f"Waiting for FlatBuffer messages... CSV logging to {csv_filename}")

    try:
        while True:
            parsed = fb_reader.read_next()
            if parsed is None:
                continue

            if parsed.payload_type == Payload.kf_debug:
                a = 1

            if parsed.payload_type == Payload.Status:
                try:
                    status = parsed.payload

                    phase_cnt = status.PhaseCnt()
                    freq_error_hz = status.FreqErrorHz()
                    freq_drift_hz_s = status.FreqDriftHzS()
                    voltage_control_v = status.VoltageControlV()
                    voltage_measured_v = status.VoltageMeasuredV()
                    temperature_c = status.TemperatureC()
                    raw_counter_value = status.RawCounterValue()

                    log.info(f"Phase (Count): {phase_cnt:.6f}")
                    log.info(f"Freq Error (Hz): {freq_error_hz:.6f}")
                    log.info(f"Freq Drift (Hz/s): {freq_drift_hz_s:.6f}")
                    log.info(f"Voltage Control (V): {voltage_control_v:.3f}")
                    log.info(f"Voltage Measured (V): {voltage_measured_v:.3f}")
                    log.info(f"Temperature (C): {temperature_c:.2f}")
                    log.info(f"Raw Counter: {raw_counter_value}")

                    timestamp = parsed.timestamp_s or (time.time() - start_time)
                    kf.predict()
                    X, P, k, y = kf.update(raw_counter_value)

                    phase_cnt_post = X[0]
                    freq_offset_post = X[1]
                    freq_drift_post = X[2]
    
                    # Log to CSV
                    csv_writer.writerow([
                        timestamp,
                        phase_cnt,
                        freq_error_hz,
                        freq_drift_hz_s,
                        voltage_control_v,
                        voltage_measured_v,
                        temperature_c,
                        raw_counter_value
                    ])
                    csv_file.flush()

                    plotter.update(timestamp, phase_cnt_post, freq_offset_post, freq_drift_post, phase_cnt, freq_error_hz, freq_drift_hz_s, voltage_control_v, voltage_measured_v)                    
                except Exception as e:
                    log.warning(f"Failed to parse Status message: {e}")
                    continue
            elif parsed.payload_type == Payload.kf_debug:
                # For now, just log key fields to confirm reception; expand handling as needed
                dbg = parsed.payload
                try:
                    state = dbg.State()
                    corr = dbg.Correction()
                    log.info(
                        f"kf_debug ts={parsed.timestamp_s:.3f} iter={dbg.KfIteration()} outliers={dbg.OutlierCount()} "
                        f"drift={state.Drift() if state else 'n/a'} "
                        f"z={corr.Z() if corr else 'n/a'}"
                    )
                except Exception as e:
                    log.warning(f"Failed to parse kf_debug payload: {e}")
                    continue
            else:
                log.info(f"Unhandled payload_type={parsed.payload_type}, msg_id={parsed.msg_id}")
                continue
    finally:
        csv_file.close()
        log.info(f"CSV file closed: {csv_filename}")
