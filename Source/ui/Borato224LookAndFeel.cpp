#include "Borato224LookAndFeel.h"

using namespace Borato224Colours;

void Borato224LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour&, bool isHighlighted, bool isDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool on = button.getToggleState();
    const float u = juce::jmax(0.55f, r.getHeight() / 60.0f);
    const float press = isDown ? 2.0f * u : 0.0f;
    r.translate(0.0f, press);

    g.setColour(juce::Colours::black.withAlpha(isDown ? 0.18f : 0.32f));
    g.fillRoundedRectangle(r.translated(0.0f, isDown ? 2.0f * u : 4.0f * u), 7.0f * u);

    juce::Colour top = isDown ? juce::Colour::fromRGB(28, 29, 32) : juce::Colour::fromRGB(76, 78, 82);
    juce::Colour bottom = isDown ? juce::Colour::fromRGB(8, 9, 10) : juce::Colour::fromRGB(15, 16, 18);
    if (isHighlighted && ! isDown)
        top = top.brighter(0.12f);

    g.setGradientFill(juce::ColourGradient(top, r.getCentreX(), r.getY(),
                                           bottom, r.getCentreX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 7.0f * u);

    g.setColour(juce::Colours::white.withAlpha(isDown ? 0.05f : 0.16f));
    g.fillRoundedRectangle(r.reduced(7.0f * u, 6.0f * u).removeFromTop(r.getHeight() * 0.30f), 4.0f * u);

    g.setColour(on ? brass().withAlpha(0.65f) : juce::Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(r.reduced(0.5f * u), 7.0f * u, (on ? 1.8f : 1.0f) * u);

    const auto ledCentre = juce::Point<float>(r.getRight() - r.getWidth() * 0.22f,
                                              r.getBottom() - r.getHeight() * 0.25f);
    const auto ledRadius = juce::jlimit(3.5f, 6.5f, r.getHeight() * 0.10f);
    drawLed(g, ledCentre, on, ledRadius);
}

void Borato224LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool isDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(5.0f);
    r.translate(0.0f, isDown ? 2.0f : 0.0f);
    r.removeFromBottom(r.getHeight() * 0.18f);

    g.setColour(juce::Colour::fromRGB(220, 196, 135));
    g.setFont(juce::Font(juce::FontOptions(juce::jlimit(11.0f, 17.0f, r.getHeight() * 0.36f))).boldened());
    g.drawFittedText(button.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
}

juce::Range<float> Borato224LookAndFeel::getFaderRulerRange(juce::Rectangle<float> bounds) noexcept
{
    const float u = juce::jmax(0.45f, juce::jmin(bounds.getWidth() / 110.0f,
                                                 bounds.getHeight() / 275.0f));
    const auto slot = bounds.withWidth(16.0f * u)
                            .withCentre({ bounds.getCentreX(), bounds.getCentreY() })
                            .reduced(0.0f, 21.0f * u);
    return { slot.getY() + 8.0f * u, slot.getBottom() - 8.0f * u };
}

void Borato224LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float, float,
                                            const juce::Slider::SliderStyle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    const float u = juce::jmax(0.45f, juce::jmin(bounds.getWidth() / 110.0f, bounds.getHeight() / 275.0f));
    auto slot = bounds.withWidth(16.0f * u)
                     .withCentre({ bounds.getCentreX(), bounds.getCentreY() })
                     .reduced(0.0f, 21.0f * u);

    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(74, 74, 70), slot.getX(), slot.getCentreY(),
                                           juce::Colour::fromRGB(18, 18, 15), slot.getCentreX(), slot.getCentreY(), false));
    g.fillRoundedRectangle(slot, 8.0f * u);

    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.drawLine(slot.getCentreX(), slot.getY() + 8.0f * u, slot.getCentreX(), slot.getBottom() - 8.0f * u, 2.0f * u);

    const auto rulerRange = getFaderRulerRange(bounds);
    for (int i = 0; i < 6; ++i)
    {
        const float ty = juce::jmap((float) i, 0.0f, 5.0f,
                                    rulerRange.getStart(), rulerRange.getEnd());
        g.setColour(juce::Colour::fromRGB(96, 90, 80));
        g.drawLine(slot.getX() - 22.0f * u, ty, slot.getX() - 8.0f * u, ty, 1.5f * u);
        g.drawLine(slot.getRight() + 8.0f * u, ty, slot.getRight() + 22.0f * u, ty, 1.5f * u);
    }

    const float capW = juce::jmin(84.0f * u, bounds.getWidth() - 4.0f * u);
    const float capH = 40.0f * u;
    const float capY = juce::jlimit(bounds.getY() + 2.0f * u, bounds.getBottom() - capH - 2.0f * u,
                                    sliderPos - capH * 0.5f);
    auto cap = juce::Rectangle<float>(bounds.getCentreX() - capW * 0.5f, capY, capW, capH);

    g.setColour(juce::Colours::black.withAlpha(0.30f));
    g.fillRoundedRectangle(cap.translated(0.0f, 4.0f * u), 7.0f * u);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(82, 84, 88), cap.getCentreX(), cap.getY(),
                                           juce::Colour::fromRGB(13, 14, 16), cap.getCentreX(), cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 7.0f * u);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.fillRoundedRectangle(cap.reduced(8.0f * u, 5.0f * u).removeFromTop(8.0f * u), 3.0f * u);
    g.setColour(juce::Colour::fromRGB(202, 202, 202));
    g.drawLine(cap.getX() + 12.0f * u, cap.getCentreY(), cap.getRight() - 12.0f * u, cap.getCentreY(), 3.0f * u);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawRoundedRectangle(cap.reduced(0.5f * u), 7.0f * u, 1.0f * u);
}

void Borato224LookAndFeel::drawLed(juce::Graphics& g, juce::Point<float> centre, bool on, float radius)
{
    if (on)
    {
        g.setColour(ledRed().withAlpha(0.20f));
        g.fillEllipse(centre.x - radius * 2.8f, centre.y - radius * 2.8f, radius * 5.6f, radius * 5.6f);
        g.setColour(ledRed());
    }
    else
    {
        g.setColour(ledOff());
    }

    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour((on ? juce::Colours::white : juce::Colour::fromRGB(162, 59, 48)).withAlpha(on ? 0.62f : 0.75f));
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
}

void Borato224LookAndFeel::drawScrew(juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour(juce::Colour::fromRGB(23, 23, 23));
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour::fromRGB(104, 104, 104));
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    g.drawLine(centre.x - radius * 0.55f, centre.y, centre.x + radius * 0.55f, centre.y, 2.0f);
}

void Borato224LookAndFeel::drawDivider(juce::Graphics& g, juce::Point<float> left,
                                       juce::Point<float> right, juce::Point<float> centre,
                                       float gapWidth, const juce::String& title, float titleHeight)
{
    const auto gap = gapWidth * 0.5f;
    g.setColour(brass());
    g.drawLine(left.x, left.y, centre.x - gap, centre.y, 1.6f);
    g.drawLine(centre.x + gap, centre.y, right.x, right.y, 1.6f);
    g.setFont(juce::Font(juce::FontOptions(titleHeight)).boldened());
    g.drawText(title, juce::Rectangle<float>(centre.x - gapWidth * 0.5f, centre.y - titleHeight,
                                             gapWidth, titleHeight * 1.4f),
               juce::Justification::centred);
}
