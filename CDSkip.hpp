#pragma once

#include <iostream>
#include <functional>
#include <random>

namespace CDSkip {

using Stereo = std::pair<float, float>;

constexpr float k_cdSampleRate = 44100;

// Expressed in samples at 44.1kHz.
constexpr int k_stereoDelay = 370;
constexpr int k_glitchLength = 360;
constexpr int k_noisePeriod = 6;

class Glitch {
public:
    Glitch(float sampleRate);
    void reset();
    float process(std::minstd_rand& rng);

private:
    const int m_noisePeriod;
    int m_phase;
};

class CDSkip {
public:
    CDSkip(float sampleRate, float maxDelay, float* memory);

    static int getMemoryLength(float sampleRate, float maxDelay);

    std::pair<float, float> process(std::pair<float, float> in);

private:
    const float m_sampleRate;
    const int m_bufferLength;
    float* const m_bufferLeft;
    float* const m_bufferRight;
    Glitch m_glitchLeft;
    Glitch m_glitchRight;

    int m_writePos;
    int m_readPosLeft;
    int m_readPosRight;
    int m_auxReadPos;
    int m_leftGlitchTimeRemaining;
    int m_rightGlitchTimeRemaining;
    int m_leftDelayTimeRemaining;

    std::minstd_rand m_rng;

    void skip(int position);
};

}
