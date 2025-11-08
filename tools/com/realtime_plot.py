import matplotlib.pyplot as plt
from collections import deque

class RealtimePlotter:
    def __init__(self, window_size=1000):
        self.window_size = window_size
        
        # Separate data storage for each measurement type
        self.timestamps = {
            'filter': deque(maxlen=window_size),
            'volt_set': deque(maxlen=window_size),
            'volt_meas': deque(maxlen=window_size),
            'freq_offset_hw': deque(maxlen=window_size)            
        }
        self.freq_offsets = deque(maxlen=window_size)
        self.freq_offsets_hw = deque(maxlen=window_size)
        self.drifts = deque(maxlen=window_size)
        self.volt_set = deque(maxlen=window_size)
        self.volt_meas = deque(maxlen=window_size)
        
        plt.ion()
        self.fig, (self.ax1, self.ax2, self.ax3) = plt.subplots(3, 1, figsize=(10, 10))
        
        # Initialize plot lines
        self.line_filtered, = self.ax1.plot([], [], 'b-', label='Frequency Offset')
        self.line_freq_offset_hw, = self.ax1.plot([], [], 'r-', label='Frequency Offset (HW)')
        self.line_drift, = self.ax2.plot([], [], 'g-', label='Frequency Drift')
        self.line_volt_set, = self.ax3.plot([], [], 'y-', label='Voltage Set')
        self.line_volt_meas, = self.ax3.plot([], [], 'm-', label='Voltage Measured')
        
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
        
    def update_filter(self, timestamp, filtered_data):
        """Update filter-related plots"""
        self.timestamps['filter'].append(timestamp)
        self.freq_offsets.append(filtered_data['freq_offset_Hz'])        
        self.drifts.append(filtered_data['drift_Hz_per_s'])
        
        x_data = list(self.timestamps['filter'])
        
        self.line_filtered.set_data(x_data, self.freq_offsets)
        self.line_drift.set_data(x_data, self.drifts)
        
        self.update_all_plots()
    
    def update_freq_offset_hw(self, timestamp, freq_offset):            
        """Update Frequency offset from Hardware Filter"""
        self.timestamps['freq_offset_hw'].append(timestamp)
        self.freq_offsets_hw.append(freq_offset)
        
        self.line_freq_offset_hw.set_data(
            list(self.timestamps['freq_offset_hw']), 
            list(self.freq_offsets_hw)
        )
        
        self.update_all_plots()

    def update_volt_set(self, timestamp, volt_set):
        """Update voltage set plot"""
        self.timestamps['volt_set'].append(timestamp)
        self.volt_set.append(volt_set)
        
        self.line_volt_set.set_data(
            list(self.timestamps['volt_set']), 
            list(self.volt_set)
        )
        
        self.update_all_plots()

    def update_volt_meas(self, timestamp, volt_meas):
        """Update voltage measured plot"""
        self.timestamps['volt_meas'].append(timestamp)
        self.volt_meas.append(volt_meas)
        
        self.line_volt_meas.set_data(
            list(self.timestamps['volt_meas']), 
            list(self.volt_meas)
        )
        
        self.update_all_plots()

    def update_all_plots(self):
        for ax in [self.ax1, self.ax2, self.ax3]:
            ax.relim()
            ax.autoscale_view()
        
        self.fig.canvas.draw()
        self.fig.canvas.flush_events()