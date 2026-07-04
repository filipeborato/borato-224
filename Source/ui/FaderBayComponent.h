#pragma once

#include <JuceHeader.h>
#include "../PluginParameters.h"

class Borato224LookAndFeel;

class FaderBayComponent : public juce::Component
{
public:
    struct FaderSpec
    {
        const char* parameterId;
        const char* title;
        const char* top;
        const char* mid;
        const char* bottom;
        const char* unit;
    };

    static constexpr std::array<FaderSpec, 8> specs {{
        { ParamIDs::decay,       "DECAY",        "12",  "2.2", "0.25", "sec" },
        { ParamIDs::bass,        "BASS",         "+12", "0",   "-12",  "dB"  },
        { ParamIDs::mid,         "MID",          "+12", "0",   "-12",  "dB"  },
        { ParamIDs::crossover,   "CROSSOVER",    "2k",  "500", "125",  "Hz"  },
        { ParamIDs::trebleDecay, "TREBLE DECAY", "+12", "0",   "-12",  "dB"  },
        { ParamIDs::depth,       "DEPTH",        "+12", "0",   "-12",  "dB"  },
        { ParamIDs::preDelay,    "PRE-DELAY",    "200", "50",  "0",    "ms"  },
        { ParamIDs::mix,         "MIX",          "100", "50",  "0",    "%"   },
    }};

    FaderBayComponent(juce::AudioProcessorValueTreeState& apvts,
                      Borato224LookAndFeel& lnf);

    void setOnFaderSelected(std::function<void(int)> cb);
    void setSelectedFader(int index);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::function<void(int)> onFaderSelected;

    std::array<juce::Slider, 8> faders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> attachments;
    int selectedFader { 0 };

    static constexpr std::array<float, 8> faderX {{ 185, 320, 455, 590, 725, 860, 995, 1235 }};
    static constexpr std::array<float, 8> scaleCentreX {{ 240, 375, 510, 645, 780, 915, 1050, 1290 }};
    static constexpr float faderY = 825.0f;
    static constexpr float faderW = 110.0f;
    static constexpr float faderH = 275.0f;
    static constexpr float designW = 1600.0f;
    static constexpr float designH = 1200.0f;

    juce::Rectangle<float> mapRect(float x, float y, float w, float h) const;
    juce::Point<float> mapPoint(float x, float y) const;
};
