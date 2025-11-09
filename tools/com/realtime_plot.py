import matplotlib.pyplot as plt
from collections import deque

class RealtimePlotter:
    def __init__(self, window_size=1000):
        self.window_size = window_size
        
        # Separate data storage for each measurement type
        self.timestamps = []

        self.freq_offset_post = deque(maxlen=window_size)
        self.freq_drift_post = deque(maxlen=window_size)
        self.freq_offset = deque(maxlen=window_size)
        self.freq_drift = deque(maxlen=window_size)
        self.volt_set = deque(maxlen=window_size)
        self.volt_meas = deque(maxlen=window_size)
                
        plt.ion()
        self.fig, (self.ax1, self.ax2, self.ax3) = plt.subplots(3, 1, figsize=(10, 10))
        
        # Initialize plot lines
        self.line_freq_offset_post, = self.ax1.plot([], [], 'b-', label='Frequency Offset (post)')
        self.line_freq_offset, = self.ax2.plot([], [], 'r-', label='Frequency Offset')

        self.line_freq_drift_post, = self.ax2.plot([], [], 'b-', label='Frequency Drift (post)')
        self.line_freq_drift, = self.ax2.plot([], [], 'r-', label='Frequency Drift')
        
        self.line_volt_set, = self.ax3.plot([], [], 'b-', label='Voltage Set')
        self.line_volt_meas, = self.ax3.plot([], [], 'r-', label='Voltage Measured')
        
        # Configure axes
        self.ax1.set_title('Frequency Offset')
        self.ax1.set_ylabel('Hz')
        self.ax1.grid(True)
        self.ax1.legend()
        
        self.ax2.set_title('Frequency Drift')
        self.ax2.set_ylabel('Hz/s')
        self.ax2.grid(True)
        self.ax2.legend()
        
        self.ax3.set_title('DAC Voltage')
        self.ax3.set_ylabel('V')
        self.ax3.grid(True)
        self.ax3.legend()
        
        plt.tight_layout()
        self.fig.canvas.draw()
        self.fig.canvas.flush_events()
        
    def update_filter(self, timestamp, freq_offset_post, freq_drift_post, freq_offset, freq_drift, volt_set, volt_meas):
        """Update filter-related plots"""
        self.timestamps.append(timestamp)
        self.freq_offset_post.append(freq_offset_post)
        self.freq_drift_post.append(freq_drift_post)
        self.freq_offset.append(freq_offset)        
        self.freq_drift.append(freq_drift)        
        self.volt_set.append(volt_set)        
        self.volt_meas.append(volt_meas)        

        x_data = list(self.timestamps)        
        
        self.line_freq_offset_post.set_data(x_data, self.freq_offset_post)
        self.line_freq_offset.set_data(x_data, self.freq_offset)
        self.line_freq_drift_post.set_data(x_data, self.freq_drift_post)
        self.line_freq_drift.set_data(x_data, self.freq_drift)
        self.line_volt_set.set_data(x_data, self.volt_set)
        self.line_volt_meas.set_data(x_data, self.volt_meas)
                
        self.update_all_plots()

    def update_all_plots(self):
        for ax in [self.ax1, self.ax2, self.ax3]:
            ax.relim()
            ax.autoscale_view()
        
        self.fig.canvas.draw()
        self.fig.canvas.flush_events()