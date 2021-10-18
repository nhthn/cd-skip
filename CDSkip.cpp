#include "CDSkip.hpp"

namespace CDSkip {

int CDSkip::getMemoryLength(float sampleRate, float maxDelay)
{
    return 2 * sampleRate * maxDelay;
}

CDSkip::CDSkip(float sampleRate, float maxDelay, float* memory)
    : m_sampleRate(sampleRate),
    m_bufferLength(sampleRate * maxDelay),
    m_bufferLeft(memory),
    m_bufferRight(memory + m_bufferLength),
    m_writePos(0),
    m_readPosLeft(0),
    m_readPosRight(0),
    m_auxReadPos(0),
    m_leftGlitchTimeRemaining(0),
    m_rightGlitchTimeRemaining(0),
    m_leftDelayTimeRemaining(0),
    m_glitchLeft(m_sampleRate),
    m_glitchRight(m_sampleRate),
    m_frozen(false),
    m_cleanMode(false),
    m_autoMode(false),
    m_autoSkipPosition(-1)
{
    for (int i = 0; i < m_bufferLength; i++) {
        m_bufferLeft[i] = 0;
        m_bufferRight[i] = 0;
    }
}

Stereo CDSkip::process(Stereo in)
{
    if (!m_frozen) {
        m_bufferLeft[m_writePos] = in.first;
        m_bufferRight[m_writePos] = in.second;
    }

    // Read after writing. That way, by calling skipRelativeToWritePos(0), a zero delay is
    // achieved.
    auto frame = std::make_pair(m_bufferLeft[m_readPosLeft], m_bufferRight[m_readPosRight]);
    auto auxFrame = std::make_pair(m_bufferLeft[m_auxReadPos], m_bufferRight[m_auxReadPos]);

    if (m_autoMode) {
        std::uniform_real_distribution<> distribution(0.0, 1.0);
        if (distribution(m_rng) < 1.0 / 10000) {
            if (m_autoSkipPosition < 0 || distribution(m_rng) < 0.1) {
                m_autoSkipPosition = static_cast<int>(distribution(m_rng) * m_bufferLength);
            }
            skip(m_autoSkipPosition);
        }
    }

    float outLeft = frame.first;
    float outRight = frame.second;

    if (m_leftDelayTimeRemaining != 0) {
        m_leftDelayTimeRemaining -= 1;
        if (m_leftDelayTimeRemaining == 0) {
            m_leftDelayTimeRemaining = 0;
            m_leftGlitchTimeRemaining = k_glitchLength * m_sampleRate / k_cdSampleRate;
        }
    }

    if (m_leftGlitchTimeRemaining != 0) {
        float k = m_glitchLeft.process(m_rng);
        outLeft = k * frame.first + (1 - k) * auxFrame.first;
        m_leftGlitchTimeRemaining -= 1;
        if (m_leftGlitchTimeRemaining == 0) {
            m_readPosLeft = m_auxReadPos;
        }
    }

    if (m_rightGlitchTimeRemaining != 0) {
        float k = m_glitchRight.process(m_rng);
        outRight = k * frame.second + (1 - k) * auxFrame.second;
        m_rightGlitchTimeRemaining -= 1;
        if (m_rightGlitchTimeRemaining == 0) {
            m_readPosRight = m_auxReadPos;
        }
    }

    if (!m_frozen) {
        m_writePos += 1;
        if (m_writePos >= m_bufferLength) {
            m_writePos = 0;
        }
    }

    m_readPosLeft += 1;
    if (m_readPosLeft >= m_bufferLength) {
        m_readPosLeft = 0;
    }

    m_readPosRight += 1;
    if (m_readPosRight >= m_bufferLength) {
        m_readPosRight = 0;
    }

    m_auxReadPos += 1;
    if (m_auxReadPos >= m_bufferLength) {
        m_auxReadPos = 0;
    }

    return std::make_pair(outLeft, outRight);
}

void CDSkip::skip(float position)
{
    if (m_cleanMode) {
        m_readPosLeft = position;
        m_readPosRight = position;
    }

    m_auxReadPos = static_cast<int>(position * m_bufferLength);
    m_auxReadPos = (m_auxReadPos % m_bufferLength + m_bufferLength) % m_bufferLength;
    m_leftDelayTimeRemaining = k_stereoDelay * m_sampleRate / k_cdSampleRate;
    m_rightGlitchTimeRemaining = k_glitchLength * m_sampleRate / k_cdSampleRate;
    m_glitchLeft.reset();
    m_glitchRight.reset();
}

void CDSkip::skipRelativeToWritePos(float offsetInSeconds)
{
    skip((static_cast<float>(m_writePos) - offsetInSeconds * m_sampleRate) / m_bufferLength);
}

Glitch::Glitch(float sampleRate)
    : m_noisePeriod(k_noisePeriod * sampleRate / k_cdSampleRate),
    m_phase(0),
    m_state(GlitchState::PositiveSpike),
    m_state2(GlitchState2::Oscillate24)
{
    reset();
}

float Glitch::process(std::minstd_rand& rng)
{
    std::uniform_real_distribution<> distribution(0, 1);

    float transitionProbability = 0.05;
    if (m_phase == 0) {
        switch (m_state2) {
        case GlitchState2::Oscillate24:
            switch (m_state) {
            case GlitchState::PositiveSpike:
                if (distribution(rng) < 0.5) {
                    m_state = GlitchState::DC2;
                } else {
                    m_state = GlitchState::Wave;
                }
                break;
            case GlitchState::DC2:
                m_state = GlitchState::NegativeSpike;
                break;
            case GlitchState::Wave:
                m_state = GlitchState::NegativeSpike;
                break;
            case GlitchState::NegativeSpike:
                m_state = GlitchState::DC1;
                break;
            default:
                m_state = GlitchState::PositiveSpike;
                if (distribution(rng) < transitionProbability) {
                    m_state2 = GlitchState2::Oscillate6;
                }
                if (distribution(rng) < transitionProbability) {
                    m_state2 = GlitchState2::Oscillate12;
                }
            }
            break;
        case GlitchState2::Oscillate12:
            switch (m_state) {
            case GlitchState::PositiveSpike:
                m_state = GlitchState::NegativeSpike;
                break;
            default:
                m_state = GlitchState::PositiveSpike;
                if (distribution(rng) < transitionProbability) {
                    m_state2 = GlitchState2::Oscillate6;
                }
                if (distribution(rng) < transitionProbability) {
                    m_state2 = GlitchState2::Oscillate24;
                }
            }
            break;
        case GlitchState2::Oscillate6:
            m_state = GlitchState::Wave;
            if (distribution(rng) < transitionProbability) {
                m_state2 = GlitchState2::Oscillate12;
            }
            if (distribution(rng) < transitionProbability) {
                m_state2 = GlitchState2::Oscillate24;
            }
            break;
        }
    }

    float out;

    switch (m_state) {
    case GlitchState::DC1:
        out = 0.5 + distribution(rng) * 0.1 - 0.05;
        break;

    case GlitchState::DC2:
        out = distribution(rng) * 0.05;
        break;

    case GlitchState::DC3:
        out = 1.0 - distribution(rng) * 0.05;
        break;

    case GlitchState::PositiveSpike:
        if (m_phase == 0) {
            out = 0.9 + 0.1 * distribution(rng);
        } else {
            out = 0.45 + distribution(rng) * 0.1;
        }
        break;

    case GlitchState::NegativeSpike:
        if (m_phase == 0) {
            out = 0.1 * distribution(rng);
        } else {
            out = 0.45 + distribution(rng) * 0.1;
        }
        break;

    case GlitchState::Noise:
        out = distribution(rng);
        break;

    case GlitchState::Wave:
        out = static_cast<float>(m_phase) / (m_noisePeriod / 2);
        if (out > 1) {
            out = 2 - out;
        }
        break;
    }

    m_phase += 1;
    if (m_phase >= m_noisePeriod) {
        m_phase = 0;
    }

    return out;
}

void Glitch::reset()
{
    m_phase = 0;
}

}
