
import tomographer
import numpy as np

param = tomographer.UniformBinsHistogram.Params(0.0, 1.0, 10)


basearray = np.array([0, 30, 80, 100, 200, 800, 1200, 600, 300, 50]);

def rand_histogram():
    u = tomographer.UniformBinsHistogram(param)
    u.load(basearray + np.random.randint(0, 500, (10,)))
    return u

a = tomographer.AveragedSimpleHistogram(param)

a.reset()
for x in range(10):
    a.addHistogram(rand_histogram())

a.finalize()

print a.prettyPrint(80)
