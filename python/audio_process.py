import pyaudio
import numpy as np
import time
from scipy.fftpack import fft
from scipy.signal import butter,filtfilt

# Constants
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100
CHUNK_SIZE = 1024
STREAM_DURATION = 5  # seconds


def record_rms():
        # Initialize PyAudio
    pa = pyaudio.PyAudio()

    # Open input streams for both microphones
    stream1 = pa.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK_SIZE,
                    input_device_index=2)  # Adjust index based on your device
    try:
        print("Streaming audio from microphone 1 for {} seconds...".format(STREAM_DURATION))
        start_time = time.time()
        data1_all = b''
        # data2_all = b''
        while time.time() - start_time < STREAM_DURATION:
            # Read audio data from both streams
            data1 = stream1.read(CHUNK_SIZE)
            # data2 = stream2.read(CHUNK_SIZE)
            data1_all += data1
            # data2_all += data2

        # Calculate RMS values from the entire data
        audio_array1 = np.frombuffer(data1_all, dtype=np.int64)
        # audio_array2 = np.frombuffer(data2_all, dtype=np.int64)

        startfreq = 1400
        cutoff = 8000 #new cutoff = 500 Hz, need a lowpass filter
        order = 5
        fs = RATE
        nyq = fs * 0.5

        def butter_highpass(cutoff, fs, order=5):
            nyquist = 0.5 * fs
            normal_cutoff = cutoff / nyquist
            b, a = butter(order, normal_cutoff, btype='high', analog=False)
            return b, a


        def butter_highpass_filter(data, cutoff, fs, order=5):
            b, a = butter_highpass(cutoff, fs, order=order)
            y = filtfilt(b, a, data)
            return y


        ## Use 1 mic and turn left and right to get rms from both sides and whichever is highest rms is way to go.
        audio_x = butter_highpass_filter(audio_array1, cutoff, fs, order)
        # audio_y = butter_highpass_filter(audio_array2, cutoff, fs, order)
        rms = np.sqrt(np.mean(audio_x ** 2))
        # rms2 = np.sqrt(np.mean(audio_y ** 2))
        print("Microphone RMS over 5 seconds:", rms)
        # print("Microphone 2 RMS over 5 seconds:", rms2)
        return rms
        

    except KeyboardInterrupt:
        print("Stopping streaming...")

    finally:
        stream1.stop_stream()
        stream1.close()
        pa.terminate()

if __name__ == "__main__":
    determine_direction()