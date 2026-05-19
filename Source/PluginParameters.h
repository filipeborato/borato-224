#pragma once

#include <JuceHeader.h>

namespace ParamIDs
{
static constexpr auto program = "program";
static constexpr auto decay = "decay";
static constexpr auto preDelay = "preDelay";
static constexpr auto bass = "bass";
static constexpr auto mid = "mid";
static constexpr auto trebleDecay = "trebleDecay";
static constexpr auto crossover = "crossover";
static constexpr auto depth = "depth";
static constexpr auto mix = "mix";
static constexpr auto input = "input";
static constexpr auto output = "output";
static constexpr auto bypass = "bypass";
}

struct ProgramPreset
{
    const char* name;
    float decay;
    float preDelayMs;
    float bassDb;
    float midDb;
    float trebleDecayDb;
    float crossoverHz;
    float depthDb;
    float mixPercent;
};

inline constexpr std::array<ProgramPreset, 8> programPresets {{
    { "HALL",   4.80f, 48.0f,  1.5f,  0.0f, -1.5f, 500.0f,  3.0f, 34.0f },
    { "ROOM",   1.15f, 18.0f,  0.0f,  1.0f, -2.5f, 650.0f, -2.0f, 26.0f },
    { "PLATE",  2.40f, 24.0f, -1.0f,  0.5f,  2.5f, 900.0f,  1.0f, 30.0f },
    { "CHMBR",  2.85f, 32.0f,  1.0f,  1.0f, -0.5f, 560.0f,  0.0f, 31.0f },
    { "AMBI",   0.55f,  9.0f, -0.5f,  0.0f, -3.5f, 750.0f, -4.0f, 18.0f },
    { "SPACE",  7.60f, 72.0f,  2.5f, -0.5f, -2.0f, 420.0f,  5.0f, 38.0f },
    { "RANDOM", 3.90f, 41.0f,  0.5f,  0.0f, -1.0f, 520.0f,  6.0f, 33.0f },
    { "USER",   2.24f, 48.0f,  0.0f,  0.0f,  0.0f, 500.0f,  0.0f, 30.0f },
}};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

