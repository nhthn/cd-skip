import warnings

import numpy as np
import scipy.signal
import soundfile

if __name__ == "__main__":
    import matplotlib.pyplot as plt

    start_frequency = 30
    end_frequency = 8000
    duration = 60 * 5

    signal, sample_rate = soundfile.read("four_notches.wav")
    signal = signal[:, 0]

    frame_size = 1024
    hop_size = frame_size // 4

    frequency_signal = []
    frequency = 0

    last_sample = 0
    time_since_last_zero_crossing = 0
    for i in range(len(signal)):
        if last_sample <= 0 and signal[i] > 0:
            if time_since_last_zero_crossing != 0:
                candidate_frequency = sample_rate / time_since_last_zero_crossing
                if start_frequency <= candidate_frequency <= end_frequency:
                    frequency = candidate_frequency
            time_since_last_zero_crossing = 0
        time_since_last_zero_crossing += 1
        frequency_signal.append(frequency)

    plt.plot(frequency_signal)
    plt.show()
