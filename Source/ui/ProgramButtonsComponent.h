#pragma once

#include <JuceHeader.h>
#include "../PluginParameters.h"

class Borato224LookAndFeel;

class ProgramButtonsComponent : public juce::Component
{
public:
    ProgramButtonsComponent(juce::AudioProcessorValueTreeState& apvts,
                            Borato224LookAndFeel& lnf);

    void setOnProgramSelected(std::function<void(int)> cb);
    void setActiveProgram(int index);
    void resized() override;

private:
    std::function<void(int)> onProgramSelected;
    std::array<juce::TextButton, 8> buttons;
    int activeProgram { 0 };

    static constexpr std::array<float, 8> xPos {{ 245, 395, 545, 695, 845, 995, 1145, 1295 }};
    static constexpr float yPos = 500.0f;
    static constexpr float buttonW = 105.0f;
    static constexpr float buttonH = 60.0f;
    static constexpr float designW = 1600.0f;

    juce::Rectangle<float> mapRect(float x, float y, float w, float h) const;
};
