import warnings

import numpy as np
import scipy.signal
import soundfile

if __name__ == "__main__":
    import matplotlib.pyplot as plt
    import sys

    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[1]} <input wav file>", file=sys.stderr)
        exit(0)

    start_frequency = 30
    end_frequency = 8000
    duration = 60 * 5

    signal, sample_rate = soundfile.read(sys.argv[1])
    signal = signal[:, 0]

    frame_size = int(sample_rate / start_frequency)
    hop_size = frame_size // 4

    frequency_signal = []
    frequency = 0

    warnings.simplefilter("ignore", np.RankWarning)

    for offset in range(0, len(signal) - frame_size, hop_size):
        frame = signal[offset : offset + frame_size] * scipy.signal.windows.hann(frame_size, False)
        magnitude_spectrum = np.abs(np.fft.rfft(frame))
        x_2 = np.argmax(magnitude_spectrum)
        x_1 = max(x_2 - 1, 0)
        x_3 = min(x_2 + 1, len(magnitude_spectrum) - 1)

        y_1 = magnitude_spectrum[x_1]
        y_2 = magnitude_spectrum[x_2]
        y_3 = magnitude_spectrum[x_3]

        quadratic_coefficients = np.polyfit([x_1, x_2, x_3], [y_1, y_2, y_3], deg=2)
        bin_ = -quadratic_coefficients[1] / (2 * quadratic_coefficients[0])
        frequency = bin_ * (sample_rate / 2) / len(magnitude_spectrum)

        frequency_signal.append(frequency)

    plt.plot(frequency_signal)
    plt.show()
