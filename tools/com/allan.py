import allantools
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

filename = r"freq_error_hz.csv"

F_OSC =  10_000_000.0

df = pd.read_csv(filename)
freq_error_hz = df["freq_error_hz"] / F_OSC

(taus, adevs, errors, ns) = allantools.oadev(freq_error_hz)

(t2, ad, ade, adn) = allantools.oadev(freq_error_hz, rate=1.0, data_type="freq")
fig = plt.loglog(t2, ad)
plt.show()