#include "ProgramButtonsComponent.h"
#include "Borato224LookAndFeel.h"
#include "../PluginParameters.h"

ProgramButtonsComponent::ProgramButtonsComponent(juce::AudioProcessorValueTreeState& /*apvts*/,
                                                 Borato224LookAndFeel& lnf)
{
    setInterceptsMouseClicks(false, true);

    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& button = buttons[i];
        button.setButtonText(programPresets[i].name);
        button.setClickingTogglesState(false);
        button.setLookAndFeel(&lnf);
        button.onClick = [this, i]
        {
            activeProgram = (int) i;
            if (onProgramSelected)
                onProgramSelected((int) i);
        };
        addAndMakeVisible(button);
    }
}

void ProgramButtonsComponent::setOnProgramSelected(std::function<void(int)> cb)
{
    onProgramSelected = std::move(cb);
}

void ProgramButtonsComponent::setActiveProgram(int index)
{
    activeProgram = index;
    for (size_t i = 0; i < buttons.size(); ++i)
        buttons[i].setToggleState((int) i == index, juce::dontSendNotification);
}

void ProgramButtonsComponent::resized()
{
    for (size_t i = 0; i < buttons.size(); ++i)
        buttons[i].setBounds(mapRect(xPos[i], yPos, buttonW, buttonH).toNearestInt());
}

juce::Rectangle<float> ProgramButtonsComponent::mapRect(float x, float y, float w, float h) const
{
    const float s = (float) getWidth() / designW;
    return { x * s, y * s, w * s, h * s };
}
