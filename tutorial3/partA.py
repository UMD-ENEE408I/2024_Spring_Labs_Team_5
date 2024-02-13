import wave
import os
import matplotlib.pyplot as plt
from scipy.io.wavfile import read
from scipy.signal import decimate
import numpy

#1

#2
human_voice = wave.open(os.getcwd()+"/ENEE408I/tutorial3/human_voice.wav", "rb")
print(human_voice.getframerate())

#3
human_voice_data = read(os.getcwd()+"/ENEE408I/tutorial3/human_voice.wav", "rb")
#plt.plot(human_voice_data[1])
#plt.xlabel("Sample #")
#plt.ylabel("Magnitude")
#plt.show()

#4
audio_data = human_voice_data[1]
downsampled = []
for i in range(14208):
    downsampled.append(audio_data[6*i])
#downsampled = decimate(audio_data, int(48000/8000))

#5
print(len(downsampled))

#6
#plt.plot(downsampled)
#plt.xlabel("Sample #")
#plt.ylabel("Magnitude")
#plt.show()

#7
#fig, ax1 = plt.subplots()
#ax1.set_ylabel("Magnitude")
#ax1.set_xlabel("Sample # (original, red)")
#ax1.plot(human_voice_data[1], color="red")
#ax1.set(xlim=(32000, 48000))

#ax2 = ax1.twiny()
#ax2.set_xlabel("Sample # (downsampled, blue)")
#ax2.plot(downsampled, color = "blue")
#ax2.set(xlim=(5333.33333333,8000))

#fig.tight_layout()
#plt.show()
