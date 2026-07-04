#pragma once

#include <JuceHeader.h>

class DisplayComponent : public juce::Component
{
public:
    struct State
    {
        juce::String mode { "MODE" };
        juce::String label { "PROGRAM" };
        juce::String value { "HALL" };
        juce::String unit;
        bool edit { false };
        bool stored { false };
    };

    void setState(const State& s);
    void paint(juce::Graphics& g) override;

private:
    State state;
};
