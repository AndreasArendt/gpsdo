import time
from gpsdo_parser import GpsdoParser
from kalman import KalmanFilter
from realtime_plot import RealtimePlotter
from gpsdo.Status import Status

MAX_FLATBUF_MESSAGE_LEN = 64

def read_loop(ser, log):
    try:
        parser = GpsdoParser()
        kf = KalmanFilter()
        plotter = RealtimePlotter()
        start_time = time.time()

        while True:
            data = ser.read(MAX_FLATBUF_MESSAGE_LEN)
            if not data or len(data) == 0:
                continue

            # Decode the message using FlatBuffers
            status = Status.GetRootAsStatus(data, 0)

            # Access fields
            print("Freq Error (Hz):", status.FreqErrorHz())
            print("Freq Drift (Hz/s):", status.FreqDirftHzDs())
            print("Voltage Control (V):", status.VoltageControlV())
            print("Voltage Measured (V):", status.VoltageMeasuredV())
            print("Temperature (C):", status.TemperatureC())
            print("-----")

            # try:
            #     line = raw.decode("utf-8", errors="replace")
            # except Exception:
            #     line = raw.decode("latin-1", errors="replace")
                
            # log.debug("recv: %s", line.strip())
            # data = parser.parse_line(line)
            
            # timestamp = time.time() - start_time
            
            # # Handle each measurement type independently
            # if data.delta is not None:
            #     filtered = kf.update(data.delta)
            #     #plotter.update_filter(timestamp, filtered)
            #     log.info(f"measurement={data.delta} filtered={filtered}")
            
            # if data.voltage_set is not None:
            #     plotter.update_volt_set(timestamp, data.voltage_set)
            #     log.info(f"volt_set={data.voltage_set}")
                
            # if data.voltage_measured is not None:
            #     plotter.update_volt_meas(timestamp, data.voltage_measured)
            #     log.info(f"volt_meas={data.voltage_measured}")
                
            # if data.freq_offset is not None:
            #     plotter.update_freq_offset_hw(timestamp, data.freq_offset)
            #     log.info(f"freq_offset={data.freq_offset}")

    except KeyboardInterrupt:
        log.info("stopping read loop by user")
    finally:
        try:
            ser.close()
        except Exception:
            pass