import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm



x = np.arange(0,2*np.pi, 0.01)
y = np.arange(0,2*np.pi, 0.01)
x, y = np.meshgrid(x, y)
Z = np.sin(np.sqrt(x**2 + y**2))

z = np.sin(np.sqrt(np.square(x)+np.square(y)))


fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
ax.plot_surface(x, y, z, vmin=z.min() * 2)

plt.show()


