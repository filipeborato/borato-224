#include "DisplayComponent.h"
#include "Borato224LookAndFeel.h"

using namespace Borato224Colours;

void DisplayComponent::setState(const State& s)
{
    state = s;
    repaint();
}

void DisplayComponent::paint(juce::Graphics& g)
{
    auto outer = getLocalBounds().toFloat();
    auto area = outer.reduced(12.0f, 11.0f);

    g.setColour(juce::Colour::fromRGB(35, 32, 29));
    g.fillRoundedRectangle(outer, 13.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(25, 0, 0), area.getCentreX(), area.getY(),
                                           juce::Colour::fromRGB(8, 0, 0), area.getCentreX(), area.getBottom(), false));
    g.fillRoundedRectangle(area, 7.0f);
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.drawRoundedRectangle(area, 7.0f, 2.0f);

    const auto sx = area.getWidth() / 1236.0f;
    const auto sy = area.getHeight() / 123.0f;
    const auto px = [area, sx](float x) { return area.getX() + x * sx; };
    const auto py = [area, sy](float y) { return area.getY() + y * sy; };

    constexpr std::array<float, 4> unitRows {{ 22.0f, 49.0f, 76.0f, 103.0f }};
    constexpr std::array<const char*, 4> unitLabels {{ "sec", "ms", "Hz", "dB" }};
    const std::array<bool, 4> unitActive {{
        state.unit == "sec",
        state.unit == "ms",
        state.unit == "Hz",
        state.unit == "dB"
    }};

    g.setColour(creamText());
    g.setFont(juce::Font(juce::FontOptions(16.0f * sy)));
    for (size_t i = 0; i < unitRows.size(); ++i)
    {
        Borato224LookAndFeel::drawLed(g, { px(48), py(unitRows[i]) }, unitActive[i], (i == 0 ? 6.4f : 5.7f) * sx);
        g.setColour(creamText().withAlpha(unitActive[i] ? 1.0f : 0.72f));
        g.drawText(unitLabels[i], juce::Rectangle<float>(px(77), py(unitRows[i] - 12.0f), 72 * sx, 23 * sy),
                   juce::Justification::centredLeft);
    }

    g.setFont(juce::Font(juce::FontOptions(17.0f * sy)).withExtraKerningFactor(0.10f));
    g.setColour(creamText());
    g.drawText(state.mode, juce::Rectangle<float>(px(210), py(22), 230 * sx, 23 * sy), juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(34.0f * sy)).boldened());
    g.drawFittedText(state.label, juce::Rectangle<float>(px(210), py(48), 270 * sx, 48 * sy).toNearestInt(),
                     juce::Justification::centredLeft, 1);

    g.setColour(ledRed().withAlpha(0.14f));
    g.setFont(juce::Font(juce::FontOptions(70.0f * sy)).withTypefaceStyle("Bold"));
    g.drawText("888888", juce::Rectangle<float>(px(492), py(20), 450 * sx, 84 * sy), juce::Justification::centredLeft);

    g.setColour(ledRed());
    g.drawText(state.value, juce::Rectangle<float>(px(492), py(20), 450 * sx, 84 * sy), juce::Justification::centredLeft);

    constexpr std::array<float, 3> statusRows {{ 31.0f, 66.0f, 101.0f }};
    constexpr std::array<const char*, 3> statusLabels {{ "Program", "Edit", "Store" }};
    const std::array<bool, 3> statusActive {{ ! state.edit, state.edit, state.stored }};
    g.setFont(juce::Font(juce::FontOptions(16.5f * sy)));
    for (size_t i = 0; i < statusRows.size(); ++i)
    {
        Borato224LookAndFeel::drawLed(g, { px(1098), py(statusRows[i]) }, statusActive[i], (i == 1 ? 6.3f : 5.8f) * sx);
        g.setColour(creamText().withAlpha(statusActive[i] ? 1.0f : 0.72f));
        g.drawText(statusLabels[i], juce::Rectangle<float>(px(1128), py(statusRows[i] - 12.0f), 100 * sx, 23 * sy),
                   juce::Justification::centredLeft);
    }
}
