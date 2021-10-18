## CDSkip

This is a project to simulate CD skipping as a real-time DSP effect. When a CD skips, it doesn't produce a simple discontinuity, but instead a complex waveform that depends on the audio being skipped from and skipped to. This is audible as a high-pitched ping in the resulting sound. It's subtle, but it's there.

CDSkip is a zero-dependency C++ library that implements fake CD skipping. You can manually control when skips happen and where they skip to, or you can set the unit on "auto mode" to generate random skips and dropouts for you. The auto mode has controllable speed. A "freeze" flag lets you disable writing, and a "clean" mode disables the glitch waveforms to give you just the clicks.

See `CDSkip.hpp` for API.

### Building

This repository contains only the core DSP (`CDSkip.cpp` and `CDSkip.hpp`) and a libsndfile frontend in `frontend.cpp`. To build on Linux:

    make

To run, use an uncompressed stereo audio file:

    ./cdskip in.wav out.wav

