#pragma once

#include <JuceHeader.h>

class ReverbEngine
{
public:
    void prepare(double sampleRate, int maximumBlockSize, int channels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, int program, float decaySeconds, float preDelayMs,
                 float bassDb, float midDb, float trebleDecayDb, float crossoverHz, float depthDb);

private:
    static constexpr int numLines = 8;
    static constexpr int numDiffusers = 4;
    static constexpr int maxChannels = 2;

    struct DelayLine
    {
        std::vector<float> buffer;
        int writePosition { 0 };

        void prepare(int samples);
        void reset();
        float read(float delaySamples) const;
        void push(float sample);
    };

    struct Allpass
    {
        DelayLine delay;
        float delaySamples { 100.0f };
        float gain { 0.55f };

        void prepare(double sampleRate, float delayMs);
        void reset();
        float process(float input);
    };

    struct ProgramShape
    {
        float diffusion;
        float density;
        float modulationRate;
        float modulationDepth;
        float size;
        float earlyLevel;
        float earlyAttackLevel;
        float stereoWidth;
        float vintageDark;
    };

    void updatePreDelay(float preDelayMs);
    void updateProgramShape(int program);
    float processPreDelayAndEarly(int channel, float input) noexcept;
    void updateRuntimeParameters(float decaySeconds, float bassDb, float midDb,
                                 float trebleDecayDb, float crossoverHz, float depthDb) noexcept;
    juce::Point<float> processTankSample(float leftIn, float rightIn) noexcept;
    static ProgramShape getProgramShape(int program) noexcept;

    double currentSampleRate { 44100.0 };
    int channelCount { 2 };
    int preDelaySamples { 0 };
    int preDelayWritePosition { 0 };
    juce::AudioBuffer<float> preDelayBuffer;

    std::array<DelayLine, numLines> tankLines;
    std::array<float, numLines> baseDelaySamples {};
    std::array<float, numLines> linePhase {};
    std::array<float, numLines> dampingState {};
    std::array<float, numLines> feedbackGain {};
    std::array<float, numLines> randomModOffset {};
    std::array<float, numLines> randomModTarget {};
    std::array<std::array<Allpass, numDiffusers>, maxChannels> inputDiffusers;

    ProgramShape currentShape { getProgramShape(0) };
    int currentProgram { -1 };
    bool isPrepared { false };
    uint32_t randomState { 0x2241978u };
    int randomCountdown { 0 };
    float currentDampCoeff { 0.1f };
    float currentTrebleGain { 1.0f };
    float currentBassGain { 1.0f };
    float currentMidGain { 1.0f };
    float currentModDepthSamples { 1.0f };
    float currentModRate { 0.15f };
    float lfoPhase { 0.0f };
    float dcInputL { 0.0f };
    float dcInputR { 0.0f };
    float dcOutputL { 0.0f };
    float dcOutputR { 0.0f };
};
