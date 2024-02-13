from scipy.io.wavfile import read
import matplotlib.pyplot as plt
from scipy.fft import fft, rfft, rfftfreq
import numpy as np
import wave
from scipy.signal import butter, lfilter
import pyaudio
from scipy.io.wavfile import write

# Question 1
input_data = read("Cafe_with_noise.wav")
fs = input_data[0]
audio = input_data[1]
# plt.plot(audio)
# plt.title("Cafe_with_noise.wav")
# plt.ylabel("Amplitude")
# plt.xlabel("Time")
# plt.show()
zaz = False
if(zaz):
    vv = wave.open("Test.wav")
    p = pyaudio.PyAudio()
    chunk = 1024
    f = vv
    # open stream
    stream = p.open(format=p.get_format_from_width(f.getsampwidth()),
                    channels=f.getnchannels(),
                    rate=f.getframerate(),
                    output=True)
    # read data
    data = f.readframes(chunk)
    # play stream
    while len(data) > 0:
        stream.write(data)
        data = f.readframes(chunk)
    # stop stream
    stream.stop_stream()
    stream.close()
    # close PyAudio
    p.terminate()

# Question 2
N = len(audio)

yf = rfft(audio)
xf = rfftfreq(N, 1 / fs)

# plt.figure(2)
# plt.plot(np.abs(yf))
#
# plt.xlim(0, 20000)  # Set the limits for the x-axis values
# plt.ylim(0, 100000000)
# plt.title("Signal in frequency domain")
#plt.show()

# Question 3
# def butter_bandpass(lowcut, highcut, fs, order=5):
#     nyq = 0.5 * fs
#     low = lowcut / nyq
#     high = highcut / nyq
#     b, a = butter(order, [low, high], btype='band')
#     return b, a
#
#
# def butter_bandpass_filter(data, lowcut, highcut, fs, order=5):
#     b, a = butter_bandpass(lowcut, highcut, fs, order=order)
#     y = lfilter(b, a, data)
#     return y
def butter_lowpass(cutoff_freq, fs, order=5):
    nyquist = 0.5 * fs
    normal_cutoff = cutoff_freq / nyquist
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    return b, a

def apply_lowpass_filter(data, cutoff_freq, fs, order=5):
    b, a = butter_lowpass(cutoff_freq, fs, order=order)
    filtered_data = lfilter(b, a, data)
    return filtered_data



vv = wave.open("Cafe_with_noise.wav")
fs = vv.getframerate()
lowcut = 20
highcut = 15000
#y = butter_bandpass_filter(audio, lowcut, highcut, fs, order=3)
# plt.plot(y)#, label='Filtered signal (%g Hz)' % f0)
# plt.xlabel('time (seconds)')
# plt.grid(True)
# plt.axis('tight')
# plt.legend(loc='upper left')
# plt.show()
y = apply_lowpass_filter(audio, 15000, 48000)
N = len(y)

yf = rfft(y)
xf = rfftfreq(N, 1 / fs)

plt.figure(2)
plt.plot(np.abs(yf))

plt.xlim(0, 20000)  # Set the limits for the x-axis values
plt.ylim(0, 100000000)
plt.title("Signal in frequency domain")
plt.show()
ww = np.int16(y)
write('Test.wav',fs,ww)
