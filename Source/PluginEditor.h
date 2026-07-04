#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Borato224LookAndFeel.h"
#include "ui/DisplayComponent.h"
#include "ui/ProgramButtonsComponent.h"
#include "ui/SystemButtonsComponent.h"
#include "ui/FaderBayComponent.h"
#include "ui/PcbComponent.h"

class Borato224AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit Borato224AudioProcessorEditor(Borato224AudioProcessor&);
    ~Borato224AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void drawPanel(juce::Graphics& g);

    juce::Rectangle<float> canvasBounds() const;
    juce::Rectangle<float> mapRect(float x, float y, float w, float h) const;
    juce::Point<float> mapPoint(float x, float y) const;

    void updateDisplayForParameter(const juce::String& id);
    void updateProgramDisplay();
    void showSystemMessage(const juce::String& label, const juce::String& value, bool stored = false);
    void endCompareIfActive();
    void nudgeSelectedParameter(float direction);
    void toggleBypassFromUi();

    static juce::String formatValue(const juce::String& id, float value);
    static juce::String displayLabelFor(const juce::String& id);
    static juce::String unitFor(const juce::String& id);

    Borato224AudioProcessor& processor;

    Borato224LookAndFeel lookAndFeel;
    PcbComponent pcb;
    DisplayComponent displayComponent;
    ProgramButtonsComponent programButtons;
    SystemButtonsComponent systemButtons;
    FaderBayComponent faderBay;

    DisplayComponent::State display;
    int selectedParameter { 0 };
    bool editMode { true };
    bool compareHeld { false };
    int storeFlashTicks { 0 };
    int transientDisplayTicks { 0 };

    static constexpr float designWidth = 1600.0f;
    static constexpr float designHeight = 1200.0f;
    static constexpr int statusHoldTicks = 18;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Borato224AudioProcessorEditor)
};
