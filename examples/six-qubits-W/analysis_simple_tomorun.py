
import numpy as np

import tomographer
import tomographer.querrorbars

final_histogram = tomographer.querrorbars.load_tomorun_csv_histogram_file('tomorun-config-Fid2-histogram.csv')

print(final_histogram.prettyPrint())

a = tomographer.querrorbars.HistogramAnalysis(final_histogram, ftox=(1,-1))

a.printFitParameters()
a.printQuantumErrorBars()
a.plot()
a.plot(log_scale=True)

# import matplotlib.pyplot as plt
# f = plt.figure()
# a = f.add_subplot(111)
# a.plot(dat[:,0],dat[:,1])
# plt.show()
