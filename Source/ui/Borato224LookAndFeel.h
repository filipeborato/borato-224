#pragma once

#include <JuceHeader.h>

namespace Borato224Colours
{
    inline juce::Colour panelBase() { return juce::Colour::fromRGB(226, 219, 204); }
    inline juce::Colour brass() { return juce::Colour::fromRGB(138, 105, 50); }
    inline juce::Colour darkText() { return juce::Colour::fromRGB(38, 38, 38); }
    inline juce::Colour creamText() { return juce::Colour::fromRGB(240, 219, 195); }
    inline juce::Colour ledRed() { return juce::Colour::fromRGB(255, 50, 31); }
    inline juce::Colour ledOff() { return juce::Colour::fromRGB(53, 5, 3); }
}

class Borato224LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool isHighlighted, bool isDown) override;
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool isDown) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float, float, const juce::Slider::SliderStyle, juce::Slider&) override;

    static juce::Range<float> getFaderRulerRange(juce::Rectangle<float> bounds) noexcept;
    static void drawLed(juce::Graphics& g, juce::Point<float> centre, bool on, float radius);
    static void drawScrew(juce::Graphics& g, juce::Point<float> centre, float radius);
    static void drawDivider(juce::Graphics& g, juce::Point<float> left, juce::Point<float> right,
                            juce::Point<float> centre, float gapWidth, const juce::String& title,
                            float titleHeight);
};
