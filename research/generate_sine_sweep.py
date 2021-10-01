import numpy as np
import soundfile

if __name__ == "__main__":
    sample_rate = 44100
    start_frequency = 30
    end_frequency = 8000
    duration = 60 * 5

    num_samples = int(sample_rate * duration)
    signal = np.zeros(num_samples)

    phase = 0
    for i in range(num_samples):
        frequency = start_frequency + (i / num_samples) * (end_frequency - start_frequency)
        phase += frequency / sample_rate
        phase = phase % 1
        signal[i] = np.sin(phase * 2 * np.pi)

    soundfile.write(
        f"sine_sweep_{start_frequency}hz_{end_frequency}hz_{duration}s.wav",
        signal,
        sample_rate,
    )

