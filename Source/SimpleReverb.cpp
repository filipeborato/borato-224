#include "SimpleReverb.h"

namespace
{
constexpr int fdnLines = 8;

constexpr std::array<float, fdnLines> baseDelayMs {{
    71.0f, 83.0f, 97.0f, 113.0f, 127.0f, 149.0f, 167.0f, 191.0f
}};

constexpr std::array<float, fdnLines> phaseOffsets {{
    0.00f, 0.37f, 0.71f, 1.13f, 1.67f, 2.11f, 2.73f, 3.19f
}};

constexpr std::array<float, 6> earlyTapMs {{ 7.0f, 13.0f, 19.0f, 29.0f, 41.0f, 53.0f }};
constexpr std::array<float, 6> earlyTapGain {{ 0.45f, -0.34f, 0.26f, -0.22f, 0.18f, -0.15f }};
constexpr std::array<std::array<float, 6>, 8> programEarlyTapScale {{
    {{ 1.30f, 1.42f, 1.58f, 1.74f, 1.92f, 2.12f }}, // HALL
    {{ 0.62f, 0.76f, 0.90f, 1.04f, 1.16f, 1.28f }}, // ROOM
    {{ 0.86f, 1.04f, 1.22f, 1.46f, 1.70f, 1.96f }}, // PLATE
    {{ 0.92f, 1.12f, 1.34f, 1.58f, 1.82f, 2.08f }}, // CHMBR
    {{ 0.42f, 0.54f, 0.68f, 0.82f, 0.96f, 1.10f }}, // AMBI
    {{ 1.55f, 1.78f, 2.04f, 2.34f, 2.68f, 3.04f }}, // SPACE
    {{ 0.82f, 1.19f, 1.37f, 1.83f, 2.16f, 2.71f }}, // RANDOM
    {{ 1.00f, 1.16f, 1.32f, 1.54f, 1.78f, 2.02f }}  // USER
}};
constexpr std::array<std::array<float, 6>, 8> programEarlyGainScale {{
    {{ 0.78f, 0.84f, 0.90f, 0.96f, 1.00f, 1.06f }},
    {{ 1.25f, 1.18f, 1.10f, 1.02f, 0.94f, 0.86f }},
    {{ 0.82f, 0.78f, 0.74f, 0.70f, 0.66f, 0.62f }},
    {{ 1.06f, 1.02f, 0.98f, 0.94f, 0.90f, 0.86f }},
    {{ 1.45f, 1.28f, 1.12f, 0.96f, 0.82f, 0.70f }},
    {{ 0.64f, 0.70f, 0.78f, 0.88f, 1.00f, 1.14f }},
    {{ 1.04f, 0.82f, 1.18f, 0.74f, 1.08f, 0.92f }},
    {{ 1.00f, 0.96f, 0.92f, 0.88f, 0.84f, 0.80f }}
}};

constexpr float earlyAttackCrossfeed = 0.18f;
constexpr float earlyAttackDirect = 1.0f - earlyAttackCrossfeed;
constexpr float minimumTailModulationScale = 0.08f;
constexpr float maximumTailModulationScale = 2.25f;
constexpr float depthModRateTrim = 0.35f;

float softLimit(float x) noexcept
{
    return x * (1.0f / (1.0f + 0.18f * std::abs(x)));
}

float onePoleCoefficient(double sampleRate, float frequencyHz) noexcept
{
    if (! std::isfinite(sampleRate) || sampleRate <= 0.0 || ! std::isfinite(frequencyHz))
        return 0.1f;

    const auto clamped = juce::jlimit(40.0f, (float) sampleRate * 0.45f, frequencyHz);
    return 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * clamped / (float) sampleRate);
}

float finiteOr(float value, float fallback) noexcept
{
    return std::isfinite(value) ? value : fallback;
}
}

void SimpleReverb::DelayLine::prepare(int samples)
{
    buffer.assign((size_t) juce::jmax(4, samples), 0.0f);
    writePosition = 0;
}

void SimpleReverb::DelayLine::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePosition = 0;
}

float SimpleReverb::DelayLine::read(float delaySamples) const
{
    const int size = (int) buffer.size();
    if (size <= 1)
        return 0.0f;

    delaySamples = juce::jlimit(1.0f, (float) size - 2.0f, finiteOr(delaySamples, 1.0f));

    const int safeWritePosition = ((writePosition % size) + size) % size;
    float readPosition = (float) safeWritePosition - delaySamples;
    while (readPosition < 0.0f)
        readPosition += (float) size;
    while (readPosition >= (float) size)
        readPosition -= (float) size;

    const int index0 = ((int) readPosition) % size;
    const int index1 = (index0 + 1) % size;
    const float frac = readPosition - (float) index0;
    return buffer[(size_t) index0] + frac * (buffer[(size_t) index1] - buffer[(size_t) index0]);
}

void SimpleReverb::DelayLine::push(float sample)
{
    const int size = (int) buffer.size();
    if (size <= 0)
        return;

    if (writePosition < 0 || writePosition >= size)
        writePosition = ((writePosition % size) + size) % size;

    buffer[(size_t) writePosition] = juce::jlimit(-8.0f, 8.0f, sample);
    if (++writePosition >= size)
        writePosition = 0;
}

void SimpleReverb::Allpass::prepare(double sampleRate, float delayMs)
{
    delaySamples = (float) sampleRate * delayMs * 0.001f;
    delay.prepare((int) std::ceil(delaySamples) + 8);
}

void SimpleReverb::Allpass::reset()
{
    delay.reset();
}

float SimpleReverb::Allpass::process(float input)
{
    const float delayed = delay.read(delaySamples);
    const float value = input + delayed * gain;
    delay.push(value);
    return delayed - value * gain;
}

void SimpleReverb::prepare(double sampleRate, int maximumBlockSize, int channels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    channelCount = juce::jlimit(1, maxChannels, channels);
    preDelaySamples = 0;
    preDelayWritePosition = 0;
    lfoPhase = 0.0f;
    dcInputL = 0.0f;
    dcInputR = 0.0f;
    dcOutputL = 0.0f;
    dcOutputR = 0.0f;
    randomState = 0x2241978u;
    randomCountdown = 0;

    const int maxPreDelaySamples = (int) std::ceil(currentSampleRate * 0.26);
    preDelayBuffer.setSize(maxChannels, maxPreDelaySamples + juce::jmax(1, maximumBlockSize) + 8, false, false, true);
    preDelayBuffer.clear();

    const int maxTankSamples = (int) std::ceil(currentSampleRate * 0.55);
    for (int i = 0; i < numLines; ++i)
    {
        tankLines[(size_t) i].prepare(maxTankSamples + 16);
        baseDelaySamples[(size_t) i] = baseDelayMs[(size_t) i] * (float) currentSampleRate * 0.001f;
        linePhase[(size_t) i] = phaseOffsets[(size_t) i];
    }

    constexpr std::array<float, numDiffusers> diffuserMs {{ 1.6f, 3.7f, 7.9f, 13.4f }};
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        for (int i = 0; i < numDiffusers; ++i)
        {
            const float offset = ch == 0 ? 1.0f : 1.17f;
            inputDiffusers[(size_t) ch][(size_t) i].prepare(currentSampleRate, diffuserMs[(size_t) i] * offset);
        }
    }

    reset();
    isPrepared = true;
}

void SimpleReverb::reset()
{
    preDelayBuffer.clear();
    preDelayWritePosition = 0;
    std::fill(dampingState.begin(), dampingState.end(), 0.0f);
    std::fill(randomModOffset.begin(), randomModOffset.end(), 0.0f);
    std::fill(randomModTarget.begin(), randomModTarget.end(), 0.0f);
    dcInputL = 0.0f;
    dcInputR = 0.0f;
    dcOutputL = 0.0f;
    dcOutputR = 0.0f;

    for (auto& line : tankLines)
        line.reset();
    for (auto& channel : inputDiffusers)
        for (auto& diffuser : channel)
            diffuser.reset();
}

void SimpleReverb::process(juce::AudioBuffer<float>& buffer, int program, float decaySeconds, float preDelayMs,
                           float bassDb, float midDb, float trebleDecayDb, float crossoverHz, float depthDb)
{
    if (! isPrepared || buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
    {
        buffer.clear();
        return;
    }

    decaySeconds = finiteOr(decaySeconds, 2.24f);
    preDelayMs = finiteOr(preDelayMs, 0.0f);
    bassDb = finiteOr(bassDb, 0.0f);
    midDb = finiteOr(midDb, 0.0f);
    trebleDecayDb = finiteOr(trebleDecayDb, 0.0f);
    crossoverHz = finiteOr(crossoverHz, 500.0f);
    depthDb = finiteOr(depthDb, 0.0f);

    updatePreDelay(preDelayMs);
    updateProgramShape(program);
    updateRuntimeParameters(decaySeconds, bassDb, midDb, trebleDecayDb, crossoverHz, depthDb);

    const int samples = buffer.getNumSamples();
    const int channels = juce::jmin(channelCount, buffer.getNumChannels());

    for (int i = 0; i < samples; ++i)
    {
        const float inL = buffer.getReadPointer(0)[i];
        const float inR = channels > 1 ? buffer.getReadPointer(1)[i] : inL;

        const float earlySourceL = processPreDelayAndEarly(0, inL);
        const float earlySourceR = processPreDelayAndEarly(1, inR);
        float tankInL = earlySourceL;
        float tankInR = earlySourceR;

        for (auto& diffuser : inputDiffusers[0])
            tankInL = diffuser.process(tankInL);
        for (auto& diffuser : inputDiffusers[1])
            tankInR = diffuser.process(tankInR);

        if (++preDelayWritePosition >= preDelayBuffer.getNumSamples())
            preDelayWritePosition = 0;

        const auto tankOut = processTankSample(tankInL, tankInR);
        const float earlyAttackL = (earlySourceL * earlyAttackDirect + earlySourceR * earlyAttackCrossfeed) * currentShape.earlyAttackLevel;
        const float earlyAttackR = (earlySourceR * earlyAttackDirect + earlySourceL * earlyAttackCrossfeed) * currentShape.earlyAttackLevel;

        buffer.getWritePointer(0)[i] = juce::jlimit(-2.0f, 2.0f, softLimit(tankOut.x + earlyAttackL));
        if (channels > 1)
            buffer.getWritePointer(1)[i] = juce::jlimit(-2.0f, 2.0f, softLimit(tankOut.y + earlyAttackR));
    }

    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, samples);
}

void SimpleReverb::updatePreDelay(float preDelayMs)
{
    if (preDelayBuffer.getNumSamples() <= 0)
    {
        preDelaySamples = 0;
        return;
    }

    const int maxDelay = juce::jmax(0, preDelayBuffer.getNumSamples() - 1);
    preDelaySamples = juce::jlimit(0, maxDelay, (int) std::round((preDelayMs * 0.001f) * (float) currentSampleRate));
}

void SimpleReverb::updateProgramShape(int program)
{
    if (program == currentProgram)
        return;

    currentProgram = program;
    currentShape = getProgramShape(program);
    randomCountdown = 0;

    for (int ch = 0; ch < maxChannels; ++ch)
        for (int i = 0; i < numDiffusers; ++i)
            inputDiffusers[(size_t) ch][(size_t) i].gain = currentShape.diffusion;
}

float SimpleReverb::processPreDelayAndEarly(int channel, float input) noexcept
{
    if (channel < 0 || channel >= preDelayBuffer.getNumChannels() || preDelayBuffer.getNumSamples() <= 1)
        return input;

    auto* delay = preDelayBuffer.getWritePointer(channel);
    const int size = preDelayBuffer.getNumSamples();
    if (preDelayWritePosition < 0 || preDelayWritePosition >= size)
        preDelayWritePosition = ((preDelayWritePosition % size) + size) % size;

    auto readIntegerDelay = [delay, size, this](int delaySamples) noexcept
    {
        delaySamples = ((delaySamples % size) + size) % size;
        const int pos = (preDelayWritePosition - delaySamples + size) % size;
        return delay[pos];
    };

    delay[preDelayWritePosition] = juce::jlimit(-8.0f, 8.0f, input);
    float delayed = readIntegerDelay(preDelaySamples);
    float early = 0.0f;

    const auto program = (size_t) juce::jlimit(0, 7, currentProgram);
    const auto& tapScale = programEarlyTapScale[program];
    const auto& gainScale = programEarlyGainScale[program];

    for (size_t i = 0; i < earlyTapMs.size(); ++i)
    {
        const int tap = (int) std::round(earlyTapMs[i] * tapScale[i] * 0.001f * (float) currentSampleRate * currentShape.size);
        const float polarity = channel == 0 ? 1.0f : ((i & 1U) != 0U ? -1.0f : 1.0f);
        early += readIntegerDelay(tap) * earlyTapGain[i] * gainScale[i] * polarity;
    }

    return delayed * 0.82f + early * currentShape.earlyLevel;
}

void SimpleReverb::updateRuntimeParameters(float decaySeconds, float bassDb, float midDb,
                                           float trebleDecayDb, float crossoverHz, float depthDb) noexcept
{
    decaySeconds = finiteOr(decaySeconds, 2.24f);
    bassDb = finiteOr(bassDb, 0.0f);
    midDb = finiteOr(midDb, 0.0f);
    trebleDecayDb = finiteOr(trebleDecayDb, 0.0f);
    crossoverHz = finiteOr(crossoverHz, 500.0f);
    depthDb = finiteOr(depthDb, 0.0f);

    decaySeconds = juce::jlimit(0.25f, 12.0f, decaySeconds);
    const float depth = juce::jlimit(0.0f, 1.0f, (depthDb + 12.0f) / 24.0f);
    const float depthCurve = std::sqrt(depth);
    const float programModDepthSamples = 0.55f + currentShape.modulationDepth * 11.0f;
    const float depthScale = minimumTailModulationScale + depthCurve * maximumTailModulationScale;
    currentModDepthSamples = programModDepthSamples * depthScale;
    currentModRate = currentShape.modulationRate * (1.0f - depthModRateTrim * 0.5f + depthCurve * depthModRateTrim);
    currentDampCoeff = onePoleCoefficient(currentSampleRate, juce::jlimit(125.0f, 2000.0f, crossoverHz));
    currentTrebleGain = juce::Decibels::decibelsToGain(juce::jlimit(-12.0f, 12.0f, trebleDecayDb) * 0.30f);
    currentBassGain = juce::Decibels::decibelsToGain(juce::jlimit(-12.0f, 12.0f, bassDb) * 0.24f);
    currentMidGain = juce::Decibels::decibelsToGain(juce::jlimit(-12.0f, 12.0f, midDb) * 0.18f);

    for (int i = 0; i < numLines; ++i)
    {
        const float delaySeconds = (baseDelaySamples[(size_t) i] * currentShape.size) / (float) currentSampleRate;
        feedbackGain[(size_t) i] = std::pow(10.0f, -3.0f * delaySeconds / decaySeconds);
    }
}

juce::Point<float> SimpleReverb::processTankSample(float leftIn, float rightIn) noexcept
{
    if (currentProgram == 6 && --randomCountdown <= 0)
    {
        randomCountdown = juce::jmax(32, (int) (currentSampleRate * 0.045));
        for (auto& target : randomModTarget)
        {
            randomState ^= randomState << 13;
            randomState ^= randomState >> 17;
            randomState ^= randomState << 5;
            const float unit = (float) (randomState & 0x00ffffffu) / (float) 0x00ffffffu;
            target = (unit * 2.0f - 1.0f) * (0.35f + currentModDepthSamples * 0.32f);
        }
    }

    std::array<float, numLines> reads {};
    for (int i = 0; i < numLines; ++i)
    {
        randomModOffset[(size_t) i] += (randomModTarget[(size_t) i] - randomModOffset[(size_t) i]) * 0.0015f;
        const float lineScale = currentShape.size * (0.92f + 0.025f * (float) i);
        const float randomDrift = currentProgram == 6 ? randomModOffset[(size_t) i] : 0.0f;
        const float mod = std::sin(lfoPhase * (1.0f + 0.071f * (float) i) + linePhase[(size_t) i]) * currentModDepthSamples
                        + randomDrift;
        reads[(size_t) i] = tankLines[(size_t) i].read(baseDelaySamples[(size_t) i] * lineScale + mod);

        auto& damp = dampingState[(size_t) i];
        damp += currentDampCoeff * (reads[(size_t) i] - damp);
        const float lowBand = damp * currentBassGain;
        const float highBand = (reads[(size_t) i] - damp) * currentTrebleGain;
        reads[(size_t) i] = softLimit((lowBand + highBand) * currentMidGain);
    }

    const float a0 = reads[0] + reads[1] + reads[2] + reads[3] + reads[4] + reads[5] + reads[6] + reads[7];
    const float a1 = reads[0] - reads[1] + reads[2] - reads[3] + reads[4] - reads[5] + reads[6] - reads[7];
    const float a2 = reads[0] + reads[1] - reads[2] - reads[3] + reads[4] + reads[5] - reads[6] - reads[7];
    const float a3 = reads[0] - reads[1] - reads[2] + reads[3] + reads[4] - reads[5] - reads[6] + reads[7];
    const float a4 = reads[0] + reads[1] + reads[2] + reads[3] - reads[4] - reads[5] - reads[6] - reads[7];
    const float a5 = reads[0] - reads[1] + reads[2] - reads[3] - reads[4] + reads[5] - reads[6] + reads[7];
    const float a6 = reads[0] + reads[1] - reads[2] - reads[3] - reads[4] - reads[5] + reads[6] + reads[7];
    const float a7 = reads[0] - reads[1] - reads[2] + reads[3] - reads[4] + reads[5] + reads[6] - reads[7];

    const std::array<float, numLines> mixed {{ a0, a1, a2, a3, a4, a5, a6, a7 }};
    const float inputMono = (leftIn + rightIn) * 0.5f;
    const float inputSide = (leftIn - rightIn) * 0.5f;
    const float densityDrive = 0.72f + currentShape.density * 0.34f;
    const float injectL = (inputMono * 0.30f + inputSide * 0.10f) * densityDrive;
    const float injectR = (inputMono * 0.30f - inputSide * 0.10f) * densityDrive;

    for (int i = 0; i < numLines; ++i)
    {
        const float feedback = mixed[(size_t) i] * 0.35355339f * feedbackGain[(size_t) i];
        const float polarity = (i & 1) == 0 ? 1.0f : -1.0f;
        const float inject = i < 4 ? injectL : injectR;
        tankLines[(size_t) i].push(softLimit(feedback + inject * polarity));
    }

    lfoPhase += (2.0f * juce::MathConstants<float>::pi * currentModRate) / (float) currentSampleRate;
    if (lfoPhase > juce::MathConstants<float>::twoPi)
        lfoPhase -= juce::MathConstants<float>::twoPi;

    constexpr float tankOutputScale = 0.35355339f;
    float left = (reads[0] + reads[2] - reads[5] + reads[7]) * tankOutputScale;
    float right = (reads[1] + reads[3] - reads[4] + reads[6]) * tankOutputScale;
    const float mid = (left + right) * 0.5f;
    const float side = (left - right) * 0.5f * currentShape.stereoWidth;
    left = mid + side;
    right = mid - side;

    constexpr float dcBlockR = 0.995f;
    const float dcFreeL = left - dcInputL + dcBlockR * dcOutputL;
    const float dcFreeR = right - dcInputR + dcBlockR * dcOutputR;
    dcInputL = left;
    dcInputR = right;
    dcOutputL = dcFreeL;
    dcOutputR = dcFreeR;

    const float vintageGain = 1.10f - currentShape.vintageDark * 0.10f;
    return {
        dcFreeL * vintageGain,
        dcFreeR * vintageGain
    };
}

SimpleReverb::ProgramShape SimpleReverb::getProgramShape(int program) noexcept
{
    switch (juce::jlimit(0, 7, program))
    {
        case 0: return { 0.66f, 0.92f, 0.18f, 0.42f, 1.18f, 0.22f, 0.11f, 1.08f, 0.36f }; // HALL
        case 1: return { 0.52f, 0.78f, 0.11f, 0.18f, 0.74f, 0.62f, 0.23f, 0.82f, 0.50f }; // ROOM
        case 2: return { 0.72f, 0.86f, 0.15f, 0.25f, 0.92f, 0.18f, 0.10f, 0.96f, 0.18f }; // PLATE
        case 3: return { 0.60f, 0.84f, 0.13f, 0.22f, 0.95f, 0.42f, 0.17f, 0.88f, 0.42f }; // CHMBR
        case 4: return { 0.45f, 0.66f, 0.08f, 0.10f, 0.54f, 0.82f, 0.28f, 0.72f, 0.62f }; // AMBI
        case 5: return { 0.74f, 0.95f, 0.21f, 0.62f, 1.42f, 0.18f, 0.08f, 1.20f, 0.34f }; // SPACE
        case 6: return { 0.69f, 0.90f, 0.27f, 0.86f, 1.05f, 0.26f, 0.14f, 1.02f, 0.44f }; // RANDOM
        default: return { 0.63f, 0.86f, 0.16f, 0.35f, 1.00f, 0.34f, 0.15f, 1.00f, 0.38f }; // USER
    }
}
