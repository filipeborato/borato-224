#include "PcbComponent.h"
#include "Borato224LookAndFeel.h"

using namespace Borato224Colours;

void PcbComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    const auto x = [area](float n) { return area.getX() + area.getWidth() * n; };
    const auto y = [area](float n) { return area.getY() + area.getHeight() * n; };
    const auto s = [area](float n) { return area.getHeight() * n; };

    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.fillRoundedRectangle(area.translated(0.0f, s(0.04f)), s(0.09f));

    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(51, 103, 70), area.getX(), area.getY(),
                                           juce::Colour::fromRGB(21, 56, 35), area.getX(), area.getBottom(), false));
    g.fillRoundedRectangle(area, s(0.10f));
    g.setColour(juce::Colour::fromRGB(9, 36, 23));
    g.drawRoundedRectangle(area.reduced(1.0f), s(0.10f), 2.2f);

    g.setColour(juce::Colours::white.withAlpha(0.045f));
    for (int i = 1; i < 18; ++i)
    {
        const float px = x((float) i / 18.0f);
        g.drawLine(px, area.getY() + 4.0f, px, area.getBottom() - 4.0f, 0.7f);
    }
    for (int i = 1; i < 7; ++i)
    {
        const float py = y((float) i / 7.0f);
        g.drawLine(area.getX() + 8.0f, py, area.getRight() - 8.0f, py, 0.7f);
    }

    auto drawPad = [&g](juce::Point<float> c, float r)
    {
        g.setColour(juce::Colour::fromRGB(218, 179, 86));
        g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour(juce::Colour::fromRGB(95, 62, 20));
        g.drawEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
        g.setColour(juce::Colour::fromRGB(32, 78, 49));
        g.fillEllipse(c.x - r * 0.34f, c.y - r * 0.34f, r * 0.68f, r * 0.68f);
    };

    auto drawTrace = [&g, &drawPad](std::initializer_list<juce::Point<float>> pts)
    {
        if (pts.size() < 2)
            return;

        juce::Path p;
        auto it = pts.begin();
        p.startNewSubPath(*it++);
        for (; it != pts.end(); ++it)
            p.lineTo(*it);

        g.setColour(juce::Colour::fromRGB(184, 128, 48).withAlpha(0.55f));
        g.strokePath(p, juce::PathStrokeType(5.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colour::fromRGB(223, 172, 75));
        g.strokePath(p, juce::PathStrokeType(2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        for (auto pt : pts)
            drawPad(pt, 4.7f);
    };

    drawTrace({ { x(0.05f), y(0.31f) }, { x(0.16f), y(0.31f) }, { x(0.16f), y(0.58f) }, { x(0.28f), y(0.58f) } });
    drawTrace({ { x(0.08f), y(0.74f) }, { x(0.23f), y(0.74f) }, { x(0.23f), y(0.41f) }, { x(0.37f), y(0.41f) } });
    drawTrace({ { x(0.43f), y(0.27f) }, { x(0.53f), y(0.27f) }, { x(0.53f), y(0.66f) }, { x(0.65f), y(0.66f) } });
    drawTrace({ { x(0.61f), y(0.78f) }, { x(0.72f), y(0.78f) }, { x(0.72f), y(0.34f) }, { x(0.86f), y(0.34f) } });
    drawTrace({ { x(0.77f), y(0.55f) }, { x(0.92f), y(0.55f) } });

    auto drawResistor = [&g, &drawPad, s](juce::Rectangle<float> r, juce::Colour body,
                                          std::array<juce::Colour, 4> bands)
    {
        const auto cy = r.getCentreY();
        g.setColour(juce::Colour::fromRGB(178, 154, 96));
        g.drawLine(r.getX() - s(0.28f), cy, r.getX(), cy, s(0.035f));
        g.drawLine(r.getRight(), cy, r.getRight() + s(0.28f), cy, s(0.035f));
        drawPad({ r.getX() - s(0.31f), cy }, s(0.055f));
        drawPad({ r.getRight() + s(0.31f), cy }, s(0.055f));

        g.setGradientFill(juce::ColourGradient(body.brighter(0.30f), r.getX(), r.getY(),
                                               body.darker(0.30f), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle(r, r.getHeight() * 0.50f);
        g.setColour(juce::Colour::fromRGB(60, 40, 24));
        g.drawRoundedRectangle(r, r.getHeight() * 0.50f, 1.0f);

        const float step = r.getWidth() / 5.5f;
        for (size_t i = 0; i < bands.size(); ++i)
        {
            g.setColour(bands[i]);
            g.fillRect(r.getX() + step * (1.05f + (float) i), r.getY() + 1.5f, step * 0.23f, r.getHeight() - 3.0f);
        }
    };

    auto drawElectrolytic = [&g, &drawPad, s](juce::Rectangle<float> r, juce::Colour colour, bool plusTop)
    {
        drawPad({ r.getCentreX(), r.getY() - s(0.10f) }, s(0.048f));
        drawPad({ r.getCentreX(), r.getBottom() + s(0.10f) }, s(0.048f));
        g.setColour(juce::Colour::fromRGB(173, 150, 91));
        g.drawLine(r.getCentreX(), r.getY() - s(0.10f), r.getCentreX(), r.getY(), s(0.020f));
        g.drawLine(r.getCentreX(), r.getBottom(), r.getCentreX(), r.getBottom() + s(0.10f), s(0.020f));

        g.setGradientFill(juce::ColourGradient(colour.brighter(0.20f), r.getX(), r.getY(),
                                               colour.darker(0.45f), r.getRight(), r.getBottom(), true));
        g.fillRoundedRectangle(r, s(0.055f));
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(r.removeFromLeft(r.getWidth() * 0.30f).reduced(2.0f), s(0.035f));
        g.setColour(juce::Colours::white.withAlpha(0.82f));
        g.setFont(juce::Font(juce::FontOptions(s(0.13f))).boldened());
        g.drawText(plusTop ? "+" : "-", r.expanded(s(0.12f)), juce::Justification::centredTop);
    };

    auto drawCeramic = [&g, &drawPad, s](juce::Point<float> c, juce::Colour colour)
    {
        drawPad({ c.x - s(0.08f), c.y + s(0.18f) }, s(0.040f));
        drawPad({ c.x + s(0.08f), c.y + s(0.18f) }, s(0.040f));
        g.setColour(juce::Colour::fromRGB(180, 154, 90));
        g.drawLine(c.x - s(0.08f), c.y + s(0.18f), c.x - s(0.05f), c.y + s(0.06f), s(0.014f));
        g.drawLine(c.x + s(0.08f), c.y + s(0.18f), c.x + s(0.05f), c.y + s(0.06f), s(0.014f));
        g.setColour(colour);
        g.fillEllipse(c.x - s(0.12f), c.y - s(0.16f), s(0.24f), s(0.28f));
        g.setColour(colour.darker(0.55f));
        g.drawEllipse(c.x - s(0.12f), c.y - s(0.16f), s(0.24f), s(0.28f), 1.0f);
    };

    auto drawDip = [&g, &drawPad, s](juce::Rectangle<float> r)
    {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(r.translated(0.0f, s(0.035f)), s(0.035f));
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(38, 38, 38), r.getX(), r.getY(),
                                               juce::Colour::fromRGB(8, 8, 8), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle(r, s(0.045f));
        g.setColour(juce::Colour::fromRGB(90, 90, 90));
        g.drawRoundedRectangle(r, s(0.045f), 1.0f);

        g.setColour(juce::Colour::fromRGB(12, 12, 12));
        g.fillEllipse(r.getX() + s(0.05f), r.getCentreY() - s(0.04f), s(0.08f), s(0.08f));

        g.setColour(juce::Colour::fromRGB(180, 180, 170));
        g.setFont(juce::Font(juce::FontOptions(s(0.12f))).boldened());
        g.drawText("LEXICON", r.removeFromTop(r.getHeight() * 0.48f), juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(s(0.10f))));
        g.drawText("224-CPU", r, juce::Justification::centred);

        const int pinsPerSide = 7;
        for (int i = 0; i < pinsPerSide; ++i)
        {
            const float t = ((float) i + 0.5f) / (float) pinsPerSide;
            const float px = r.getX() + r.getWidth() * t;
            g.setColour(juce::Colour::fromRGB(198, 178, 120));
            g.drawLine(px, r.getY(), px, r.getY() - s(0.105f), s(0.020f));
            g.drawLine(px, r.getBottom(), px, r.getBottom() + s(0.105f), s(0.020f));
            drawPad({ px, r.getY() - s(0.13f) }, s(0.032f));
            drawPad({ px, r.getBottom() + s(0.13f) }, s(0.032f));
        }
    };

    auto drawToggle = [&g, &drawPad, s](juce::Rectangle<float> r, bool up)
    {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(r.translated(0.0f, s(0.025f)), s(0.035f));
        g.setColour(juce::Colour::fromRGB(18, 18, 18));
        g.fillRoundedRectangle(r, s(0.045f));
        g.setColour(juce::Colour::fromRGB(92, 92, 88));
        g.drawRoundedRectangle(r, s(0.045f), 1.2f);

        auto well = r.reduced(s(0.055f), s(0.055f));
        g.setColour(juce::Colour::fromRGB(40, 40, 40));
        g.fillRoundedRectangle(well, s(0.030f));

        const auto pivot = well.getCentre();
        const auto tip = juce::Point<float>(pivot.x + (up ? s(0.12f) : -s(0.12f)),
                                            pivot.y + (up ? -s(0.16f) : s(0.16f)));
        g.setColour(juce::Colour::fromRGB(12, 12, 12));
        g.fillEllipse(pivot.x - s(0.10f), pivot.y - s(0.10f), s(0.20f), s(0.20f));
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(238, 232, 218), pivot.x, pivot.y,
                                               juce::Colour::fromRGB(145, 137, 122), tip.x, tip.y, false));
        g.drawLine(pivot.x, pivot.y, tip.x, tip.y, s(0.055f));
        g.setColour(juce::Colour::fromRGB(218, 198, 132));
        drawPad({ r.getCentreX() - s(0.18f), r.getBottom() + s(0.09f) }, s(0.032f));
        drawPad({ r.getCentreX() + s(0.18f), r.getBottom() + s(0.09f) }, s(0.032f));
    };

    drawResistor({ x(0.09f), y(0.36f), s(0.62f), s(0.18f) }, juce::Colour::fromRGB(198, 170, 114),
                 { juce::Colour::fromRGB(120, 38, 30), juce::Colours::black, juce::Colour::fromRGB(198, 153, 43), juce::Colour::fromRGB(120, 90, 28) });
    drawResistor({ x(0.30f), y(0.18f), s(0.58f), s(0.17f) }, juce::Colour::fromRGB(190, 154, 100),
                 { juce::Colour::fromRGB(52, 88, 148), juce::Colour::fromRGB(120, 38, 30), juce::Colours::black, juce::Colour::fromRGB(198, 153, 43) });
    drawResistor({ x(0.70f), y(0.60f), s(0.60f), s(0.18f) }, juce::Colour::fromRGB(205, 178, 124),
                 { juce::Colour::fromRGB(50, 120, 58), juce::Colour::fromRGB(120, 38, 30), juce::Colours::black, juce::Colour::fromRGB(198, 153, 43) });

    drawElectrolytic({ x(0.79f), y(0.18f), s(0.22f), s(0.45f) }, juce::Colour::fromRGB(38, 62, 136), true);
    drawElectrolytic({ x(0.88f), y(0.33f), s(0.20f), s(0.38f) }, juce::Colour::fromRGB(28, 105, 135), false);
    drawCeramic({ x(0.57f), y(0.38f) }, juce::Colour::fromRGB(177, 123, 45));
    drawCeramic({ x(0.64f), y(0.36f) }, juce::Colour::fromRGB(160, 72, 38));

    drawDip({ x(0.40f), y(0.39f), s(0.90f), s(0.34f) });

    drawToggle({ x(0.53f), y(0.12f), s(0.54f), s(0.46f) }, true);
    drawToggle({ x(0.60f), y(0.12f), s(0.54f), s(0.46f) }, false);
    drawToggle({ x(0.67f), y(0.12f), s(0.54f), s(0.46f) }, true);

    g.setColour(juce::Colours::white.withAlpha(0.50f));
    g.setFont(juce::Font(juce::FontOptions(s(0.095f))).withExtraKerningFactor(0.08f));
    g.drawText("REV 224 LOGIC / MEMORY", juce::Rectangle<float> { x(0.015f), y(0.06f), s(2.1f), s(0.16f) },
               juce::Justification::centredLeft);
    g.drawText("ON", juce::Rectangle<float> { x(0.53f), y(0.01f), s(0.54f), s(0.12f) }, juce::Justification::centred);
    g.drawText("MOD", juce::Rectangle<float> { x(0.60f), y(0.01f), s(0.54f), s(0.12f) }, juce::Justification::centred);
    g.drawText("MEM", juce::Rectangle<float> { x(0.67f), y(0.01f), s(0.54f), s(0.12f) }, juce::Justification::centred);

    g.setColour(juce::Colours::black.withAlpha(0.55f));
    for (auto c : { juce::Point<float> { x(0.02f), y(0.27f) }, juce::Point<float> { x(0.98f), y(0.27f) },
                    juce::Point<float> { x(0.02f), y(0.74f) }, juce::Point<float> { x(0.98f), y(0.74f) } })
    {
        g.fillEllipse(c.x - s(0.07f), c.y - s(0.07f), s(0.14f), s(0.14f));
        g.setColour(juce::Colour::fromRGB(111, 143, 114));
        g.drawEllipse(c.x - s(0.07f), c.y - s(0.07f), s(0.14f), s(0.14f), 1.1f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
    }
}
