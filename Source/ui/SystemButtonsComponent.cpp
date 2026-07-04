#include "SystemButtonsComponent.h"
#include "Borato224LookAndFeel.h"

SystemButtonsComponent::SystemButtonsComponent(Borato224LookAndFeel& lnf)
{
    setInterceptsMouseClicks(false, true);

    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& button = buttons[i];
        button.setButtonText(labels[i]);
        button.setClickingTogglesState(false);
        button.setLookAndFeel(&lnf);
        addAndMakeVisible(button);
    }

    buttons[0].onClick = [this] { if (callbacks.onStore) callbacks.onStore(); };
    buttons[1].onClick = [this] { if (callbacks.onRecall) callbacks.onRecall(); };
    buttons[2].onClick = [this] { if (callbacks.onSwapAB) callbacks.onSwapAB(); };
    buttons[3].onClick = [this] { if (callbacks.onToggleCompare) callbacks.onToggleCompare(); };
    buttons[4].onClick = [this] { if (callbacks.onToggleEdit) callbacks.onToggleEdit(); };
    buttons[5].onClick = [this] { if (callbacks.onToggleBypass) callbacks.onToggleBypass(); };
    buttons[6].onClick = [this] { if (callbacks.onNudgeDown) callbacks.onNudgeDown(); };
    buttons[7].onClick = [this] { if (callbacks.onNudgeUp) callbacks.onNudgeUp(); };
}

void SystemButtonsComponent::setCallbacks(Callbacks cb)
{
    callbacks = std::move(cb);
}

void SystemButtonsComponent::setButtonStates(bool abSlot, bool compareHeld, bool editMode,
                                              bool bypassOn, bool storeFlash)
{
    buttons[0].setToggleState(storeFlash, juce::dontSendNotification);
    buttons[2].setToggleState(abSlot, juce::dontSendNotification);
    buttons[3].setToggleState(compareHeld, juce::dontSendNotification);
    buttons[4].setToggleState(editMode, juce::dontSendNotification);
    buttons[5].setToggleState(bypassOn, juce::dontSendNotification);
}

void SystemButtonsComponent::resized()
{
    for (size_t i = 0; i < buttons.size(); ++i)
        buttons[i].setBounds(mapRect(xPos[i], yPos, buttonW, buttonH).toNearestInt());
}

juce::Rectangle<float> SystemButtonsComponent::mapRect(float x, float y, float w, float h) const
{
    const float s = (float) getWidth() / designW;
    return { x * s, y * s, w * s, h * s };
}
