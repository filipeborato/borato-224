#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class Borato224AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit Borato224AudioProcessorEditor(Borato224AudioProcessor&);
    ~Borato224AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class HardwareButtonLookAndFeel;
    class FaderLookAndFeel;

    struct DisplayState
    {
        juce::String mode { "MODE" };
        juce::String label { "PROGRAM" };
        juce::String value { "HALL" };
        juce::String unit;
        bool edit { false };
        bool stored { false };
    };

    struct FaderSpec
    {
        const char* parameterId;
        const char* title;
        const char* top;
        const char* mid;
        const char* bottom;
        const char* unit;
    };

    void timerCallback() override;
    void configureProgramButtons();
    void configureSystemButtons();
    void configureFaders();
    void refreshButtonStates();
    void setDisplayForParameter(const juce::String& id);
    void updateProgramDisplay();
    void nudgeSelectedParameter(float direction);
    void toggleBypassFromUi();

    juce::Rectangle<float> canvasBounds() const;
    juce::Rectangle<float> mapRect(float x, float y, float w, float h) const;
    juce::Point<float> mapPoint(float x, float y) const;

    void drawPanel(juce::Graphics& g);
    void drawPcb(juce::Graphics& g, juce::Rectangle<float> area);
    void drawScrew(juce::Graphics& g, juce::Point<float> centre, float radius);
    void drawDivider(juce::Graphics& g, float y, const juce::String& title);
    void drawDisplay(juce::Graphics& g, juce::Rectangle<float> area);
    void drawLed(juce::Graphics& g, juce::Point<float> centre, bool on, float radius);
    void drawFaderScales(juce::Graphics& g);

    static juce::String formatValue(const juce::String& id, float value);
    static juce::String displayLabelFor(const juce::String& id);
    static juce::String unitFor(const juce::String& id);

    Borato224AudioProcessor& processor;

    std::unique_ptr<HardwareButtonLookAndFeel> buttonLookAndFeel;
    std::unique_ptr<FaderLookAndFeel> faderLookAndFeel;

    std::array<juce::TextButton, 8> programButtons;
    std::array<juce::TextButton, 8> systemButtons;
    std::array<juce::Slider, 8> faders;

    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> faderAttachments;

    DisplayState display;
    int selectedParameter { 0 };
    bool editMode { true };
    bool compareHeld { false };
    bool storeFlash { false };

    static constexpr float designWidth = 1600.0f;
    static constexpr float designHeight = 1200.0f;

    static constexpr std::array<const char*, 8> systemLabels {{
        "STORE", "RECALL", "A/B", "COMPARE", "EDIT", "BYPASS", "VALUE -", "VALUE +"
    }};

    static constexpr std::array<FaderSpec, 8> faderSpecs {{
        { ParamIDs::decay,       "DECAY",        "12",  "2.2", "0.25", "sec" },
        { ParamIDs::bass,        "BASS",         "+12", "0",   "-12",  "dB"  },
        { ParamIDs::mid,         "MID",          "+12", "0",   "-12",  "dB"  },
        { ParamIDs::crossover,   "CROSSOVER",    "2k",  "500", "125",  "Hz"  },
        { ParamIDs::trebleDecay, "TREBLE DECAY", "+12", "0",   "-12",  "dB"  },
        { ParamIDs::depth,       "DEPTH",        "+12", "0",   "-12",  "dB"  },
        { ParamIDs::preDelay,    "PRE-DELAY",    "200", "50",  "0",    "ms"  },
        { ParamIDs::mix,         "MIX",          "100", "50",  "0",    "%"   },
    }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Borato224AudioProcessorEditor)
};

