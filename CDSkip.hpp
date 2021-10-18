#pragma once

#include <functional>
#include <random>

namespace CDSkip {

using Stereo = std::pair<float, float>;

constexpr float k_cdSampleRate = 44100;

// Expressed in samples at 44.1kHz.
constexpr int k_stereoDelay = 150;
constexpr int k_glitchLength = 360;
constexpr int k_noisePeriod = 6;
constexpr int k_dropoutLength = 3973;
constexpr int k_samplesBeforeGlitch = 691;
constexpr int k_samplesAfterGlitch = 686 + k_glitchLength - k_stereoDelay;

enum class GlitchState {
    DC1,
    DC2,
    DC3,
    Noise,
    PositiveSpike,
    NegativeSpike,
    Wave
};

enum class GlitchState2 {
    Oscillate24, // 24-sample oscillation = 1.837 kHz
    Oscillate12, // 12-sample oscillation = 3.675 kHz
    Oscillate6 // 6-sample oscillation = 7.35 kHz
};

class Glitch {
public:
    Glitch(float sampleRate);
    void reset();
    float process(std::minstd_rand& rng);

private:
    const int m_noisePeriod;
    int m_phase;
    GlitchState m_state;
    GlitchState2 m_state2;
};

enum class AutoState {
    BeforeGlitch,
    AfterGlitch,
    Dropout
};

/** An audio unit that emulates CD skipping on stereo signals. Its behavior is based on
 * analysis of waveforms produced by played scratched CDs on a cheap CD player. It can
 * operate in two modes -- automatic, where random skips and glitches are applied for you,
 * and manual, where you can manually trigger skips to arbitrary points.
 */
class CDSkip {
public:
    CDSkip(float sampleRate, float maxDelay, float* memory, uint32_t seed = 0);

    static int getMemoryLength(float sampleRate, float maxDelay);

    /** Process a single stereo sample of output.
     */
    std::pair<float, float> process(std::pair<float, float> in);

    /** Skip to a position in the buffer. Positions are arbitrary, but skipping to the
     * same position multiple times results in the classic repeat effect. Rather than a
     * basic discontinuity, a glitch is added that crossfades the two signals in a weird
     * and noisy way. */
    void skip(float position);
    /** Skip to a position behind the write position. For example, skipping to 0 resets
     * the read position so the read and write heads are aligned. Skipping to 0.1 creates
     * a 0.1-second delay. */
    void skipRelativeToWritePos(float offsetInSeconds);

    /** Enable or disable freeze mode. In freeze mode, writing is disabled. */
    void setFrozen(bool frozen) { m_frozen = frozen; }

    /** Enable or disable clean mode, where skips are simply discontinuities without added
      * glitches. */
    void setCleanMode(bool cleanMode) { m_cleanMode = cleanMode; }

    /** Enable or disable auto mode, where random skips are added for you. */
    void setAutoMode(bool autoMode) { m_autoMode = autoMode; }

    /** Set the speed of the auto multiplier. */
    void setAutoModeSpeed(float speed) { m_autoTimeMultiplier = 1 / speed; }

private:
    const float m_sampleRate;
    const int m_bufferLength;
    float* const m_bufferLeft;
    float* const m_bufferRight;
    Glitch m_glitchLeft;
    Glitch m_glitchRight;

    std::minstd_rand m_rng;

    bool m_frozen;
    bool m_cleanMode;
    bool m_autoMode;

    int m_writePos;
    int m_readPosLeft;
    int m_readPosRight;
    int m_auxReadPos;
    int m_leftGlitchTimeRemaining;
    int m_rightGlitchTimeRemaining;
    int m_leftDelayTimeRemaining;

    float m_autoSkipPosition;
    int m_autoTimeRemaining;
    AutoState m_autoState;
    float m_autoTimeMultiplier;
};

}
