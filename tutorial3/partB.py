from scipy.io import wavfile
import matplotlib.pyplot as plt
import numpy as np

# calculating RMS
sample_rate1, data1 = wavfile.read('Lab3/M1.wav')
sample_rate2, data2 = wavfile.read('Lab3/M2.wav')
sample_rate3, data3 = wavfile.read('Lab3/M3.wav')

data1squared = np.array([int(data1[i]**2) for i in range(len(data1))])
data2squared = np.array([int(data2[i]**2) for i in range(len(data2))])
data3squared = np.array([int(data3[i]**2) for i in range(len(data3))])

rms1 = np.sqrt(np.mean(data1squared))
rms2 = np.sqrt(np.mean(data2squared))
rms3 = np.sqrt(np.mean(data3squared))

print('rms1: ' + str(rms1))
print('rms2: ' + str(rms2))
print('rms3: ' + str(rms3))


# calculating correlation
x = data1[8000:12000]
y = data1[8000:12000] # using only a slice of the audio to reduce computation time

R = np.zeros([len(x)])

for m in range(len(R)):
    for n in range(len(y)):
        R[m] += x[n] * y[n - m]

max_index = np.argmax(R)
print("delay: " + str(max_index / sample_rate1))
