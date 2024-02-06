from scipy.optimize import fsolve
from scipy.optimize import minimize
import numpy as np
from scipy.fft import fft
import matplotlib.pyplot as plt
def equations(p):
    x, y = p
    return (3*x+y-9, x+2*y-8)

x, y =  fsolve(equations, (1, 1))

print("Question 1:")
print(x,y)

def q2eqn(p):
    return p**2+2*p

res = minimize(q2eqn, x0=0)
print("Question 2:")
print("min x: ",res.x," min y: ",res.fun)

inx = np.arange(0,0.2,0.01)
outy = np.sin(100*np.pi*inx)+0.5*np.sin(160*np.pi*inx)
sigy = fft(outy)
plt.plot(np.abs(sigy))
plt.title("Signal Response")
plt.xlabel("Frequency")
plt.ylabel("Absolute Value")
plt.show()