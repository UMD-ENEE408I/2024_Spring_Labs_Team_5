import matplotlib.pyplot as plt
import numpy as np
from scipy.fftpack import fft
from scipy.io import wavfile
from scipy.signal import butter,filtfilt, lfilter
from scipy.io.wavfile import write

sample_rate, audio_time_series = wavfile.read('Cafe_with_noise.wav')

def fft_plot(audio, sample_rate):
  N = len(audio)    # Number of samples
  T = 1/sample_rate # Period
  y_freq = fft(audio)
  domain = len(y_freq) // 2
  x_freq = np.linspace(0, sample_rate//2, N//2)
  plt.plot(x_freq, abs(y_freq[:domain]))
  plt.xlabel("Frequency [Hz]")
  plt.ylabel("Frequency Amplitude |X(t)|")
  return plt.show()

fft_plot(audio_time_series, sample_rate)

startfreq = 1400
cutoff = 1600
order = 5
fs = sample_rate
nyq = fs * 0.5

def butter_lowpass_filter(data, cutoff, fs, order):
    normal_cutoff = cutoff / nyq
    normal_startfreq = startfreq / nyq
    bandfilt = np.array([normal_startfreq, normal_cutoff])

    # Get the filter coefficients
    b, a = butter(order, bandfilt, btype='bandstop', analog=False)
    y = filtfilt(b, a, data)
    return y

y = butter_lowpass_filter(audio_time_series, cutoff, fs, order)

m = np.max(np.abs(y))
y = (y/m).astype(np.float32)

fft_plot(y, sample_rate)
wavfile.write('test2.wav', sample_rate, y)