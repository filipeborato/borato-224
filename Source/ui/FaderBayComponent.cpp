#include "FaderBayComponent.h"
#include "Borato224LookAndFeel.h"

using namespace Borato224Colours;

FaderBayComponent::FaderBayComponent(juce::AudioProcessorValueTreeState& apvtsRef,
                                     Borato224LookAndFeel& lnf)
    : apvts(apvtsRef)
{
    setInterceptsMouseClicks(false, true);

    for (size_t i = 0; i < faders.size(); ++i)
    {
        auto& fader = faders[i];
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader.setLookAndFeel(&lnf);
        fader.setVelocityBasedMode(true);
        fader.setVelocityModeParameters(0.65, 1, 0.08, true);
        fader.setMouseDragSensitivity(260);

        if (auto* parameter = apvts.getParameter(specs[i].parameterId))
            fader.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

        addAndMakeVisible(fader);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, specs[i].parameterId, fader);
    }
}

void FaderBayComponent::setOnFaderSelected(std::function<void(int)> cb)
{
    onFaderSelected = std::move(cb);

    for (size_t i = 0; i < faders.size(); ++i)
    {
        faders[i].onDragStart = [this, i]
        {
            selectedFader = (int) i;
            if (onFaderSelected)
                onFaderSelected((int) i);
        };

        faders[i].onValueChange = [this, i]
        {
            if (! faders[i].isMouseButtonDown())
                return;

            selectedFader = (int) i;
            if (onFaderSelected)
                onFaderSelected((int) i);
        };
    }
}

void FaderBayComponent::setSelectedFader(int index)
{
    selectedFader = index;
}

void FaderBayComponent::paint(juce::Graphics& g)
{
    for (size_t i = 0; i < specs.size(); ++i)
    {
        const auto& spec = specs[i];
        auto title = mapRect(scaleCentreX[i] - 86.0f, 782.0f, 172.0f, 36.0f);
        g.setColour(brass());
        const bool twoLineTitle = juce::String(spec.title) == "TREBLE DECAY";
        g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, twoLineTitle ? 18.0f : 22.0f).getHeight())).boldened());
        g.drawFittedText(spec.title, title.toNearestInt(), juce::Justification::centred, 2);

        const auto labelW = mapRect(0.0f, 0.0f, 48.0f, 1.0f).getWidth();
        const auto labelH = mapRect(0.0f, 0.0f, 1.0f, 22.0f).getHeight();
        const auto labelX = mapPoint(scaleCentreX[i] - 70.0f, 0.0f).x;
        const auto rulerRange = Borato224LookAndFeel::getFaderRulerRange(faders[i].getBounds().toFloat());
        const std::array<float, 3> labelCentres {{
            rulerRange.getStart(),
            rulerRange.getStart() + rulerRange.getLength() * 0.5f,
            rulerRange.getEnd()
        }};

        g.setColour(darkText().withAlpha(0.82f));
        g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 16).getHeight())));
        g.drawText(spec.top, juce::Rectangle<float>(labelX, labelCentres[0] - labelH * 0.5f, labelW, labelH),
                   juce::Justification::centredRight);
        g.drawText(spec.mid, juce::Rectangle<float>(labelX, labelCentres[1] - labelH * 0.5f, labelW, labelH),
                   juce::Justification::centredRight);
        g.drawText(spec.bottom, juce::Rectangle<float>(labelX, labelCentres[2] - labelH * 0.5f, labelW, labelH),
                   juce::Justification::centredRight);

        g.setColour(darkText().withAlpha(0.75f));
        g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 15).getHeight())));
        const auto unitW = mapRect(0.0f, 0.0f, 56.0f, 1.0f).getWidth();
        const auto unitH = mapRect(0.0f, 0.0f, 1.0f, 18.0f).getHeight();
        const auto unitGap = mapRect(0.0f, 0.0f, 1.0f, 14.0f).getHeight();
        g.drawText(spec.unit,
                   juce::Rectangle<float>(mapPoint(scaleCentreX[i], 0.0f).x - unitW * 0.5f,
                                          rulerRange.getEnd() + unitGap, unitW, unitH),
                   juce::Justification::centred);
    }
}

void FaderBayComponent::resized()
{
    for (size_t i = 0; i < faders.size(); ++i)
        faders[i].setBounds(mapRect(faderX[i], faderY, faderW, faderH).toNearestInt());
}

juce::Rectangle<float> FaderBayComponent::mapRect(float x, float y, float w, float h) const
{
    const float s = (float) getWidth() / designW;
    return { x * s, y * s, w * s, h * s };
}

juce::Point<float> FaderBayComponent::mapPoint(float x, float y) const
{
    const float s = (float) getWidth() / designW;
    return { x * s, y * s };
}
