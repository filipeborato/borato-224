#pragma once

#include <JuceHeader.h>

class Borato224LookAndFeel;

class SystemButtonsComponent : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void()> onStore;
        std::function<void()> onRecall;
        std::function<void()> onSwapAB;
        std::function<void()> onToggleCompare;
        std::function<void()> onToggleEdit;
        std::function<void()> onToggleBypass;
        std::function<void()> onNudgeDown;
        std::function<void()> onNudgeUp;
    };

    SystemButtonsComponent(Borato224LookAndFeel& lnf);

    void setCallbacks(Callbacks cb);
    void setButtonStates(bool abSlot, bool compareHeld, bool editMode,
                         bool bypassOn, bool storeFlash);
    void resized() override;

private:
    Callbacks callbacks;
    std::array<juce::TextButton, 8> buttons;

    static constexpr std::array<const char*, 8> labels {{
        "STORE", "RECALL", "A/B", "COMPARE", "EDIT", "BYPASS", "VALUE -", "VALUE +"
    }};
    static constexpr std::array<float, 8> xPos {{ 245, 395, 545, 695, 845, 995, 1145, 1295 }};
    static constexpr float yPos = 650.0f;
    static constexpr float buttonW = 120.0f;
    static constexpr float buttonH = 62.0f;
    static constexpr float designW = 1600.0f;

    juce::Rectangle<float> mapRect(float x, float y, float w, float h) const;
};
