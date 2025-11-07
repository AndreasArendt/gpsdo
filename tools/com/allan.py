import allantools
import numpy as np
import matplotlib.pyplot as plt
import re

filename = r"reader1.log"

delta = np.array([])

with open(filename) as file:
        while line := file.readline():
            _delta_re = re.compile(r"measurement=(\d+(?:\.\d+)?)", re.IGNORECASE)
            m = _delta_re.search(line.strip())

            if m:
                delta = np.append(delta, float(m.group(1)) * 1/625000)

(taus, adevs, errors, ns) = allantools.oadev(delta)

t = np.logspace(0, 3, 50)  # tau values from 1 to 1000
r = 1  # sample rate in Hz of the input data
(t2, ad, ade, adn) = allantools.oadev(delta, rate=r, data_type="freq", taus=t)  # Compute the overlapping ADEV
fig = plt.loglog(t2, ad) # Plot the results
plt.show()