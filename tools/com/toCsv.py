from gpsdo_parser import GpsdoParser
import matplotlib.pyplot as plt
import numpy as np

filename = r"reader3.log"

parser = GpsdoParser()

measurement = np.array([])

with open(filename) as file:
    while line := file.readline():
        data = parser.parse_line(line)
        
        if data.measurement is not None:
            print(data.measurement)
            measurement = np.append(measurement, data.measurement)

plt.plot(measurement)
plt.show()