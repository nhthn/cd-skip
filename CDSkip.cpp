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
    m_glitchRight(m_sampleRate)
{
    for (int i = 0; i < m_bufferLength; i++) {
        m_bufferLeft[i] = 0;
        m_bufferRight[i] = 0;
    }
}

Stereo CDSkip::process(Stereo in)
{
    m_bufferLeft[m_writePos] = in.first;
    m_bufferRight[m_writePos] = in.second;

    auto frame = std::make_pair(m_bufferLeft[m_readPosLeft], m_bufferRight[m_readPosRight]);
    auto auxFrame = std::make_pair(m_bufferLeft[m_auxReadPos], m_bufferRight[m_auxReadPos]);

    std::uniform_real_distribution<> distribution(0.0, 1.0);
    if (distribution(m_rng) < 1.0 / 10000) {
        skip(static_cast<int>(distribution(m_rng) * m_bufferLength));
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

    m_writePos += 1;
    if (m_writePos >= m_bufferLength) {
        m_writePos = 0;
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

void CDSkip::skip(int position)
{
    m_auxReadPos = position;
    m_leftDelayTimeRemaining = k_stereoDelay * m_sampleRate / k_cdSampleRate;
    m_rightGlitchTimeRemaining = k_glitchLength * m_sampleRate / k_cdSampleRate;
    m_glitchLeft.reset();
    m_glitchRight.reset();
}

Glitch::Glitch(float sampleRate)
    : m_noisePeriod(k_noisePeriod * sampleRate / k_cdSampleRate),
    m_phase(0),
    m_state(GlitchState::Spike)
{
    reset();
}

float Glitch::process(std::minstd_rand& rng)
{
    if (m_phase == 0) {
        std::uniform_int_distribution<> stateDistribution(0, 3);
        switch (stateDistribution(rng)) {
        case 0:
            m_state = GlitchState::Silence;
            break;
        case 1:
            m_state = GlitchState::Noise;
            break;
        case 2:
            m_state = GlitchState::Spike;
            break;
        }
    }

    std::uniform_real_distribution<> distribution(0, 1);

    float out;

    switch (m_state) {
    case GlitchState::Spike:
        float result;
        if (m_phase == 0) {
            out = 1 - distribution(rng) * 0.1;
        } else {
            out = distribution(rng) * 0.1;
        }
        break;

    case GlitchState::Noise:
        out = distribution(rng);
        break;

    case GlitchState::Silence:
        out = 0.5 + distribution(rng) * 0.1 - 0.05;
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
