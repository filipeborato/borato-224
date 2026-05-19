#include "PluginEditor.h"

namespace
{
constexpr auto panelTop = 110.0f;
constexpr auto panelLeft = 90.0f;
constexpr auto panelWidth = 1420.0f;
constexpr auto panelHeight = 1075.0f;

juce::Colour panelBase() { return juce::Colour::fromRGB(226, 219, 204); }
juce::Colour brass() { return juce::Colour::fromRGB(138, 105, 50); }
juce::Colour darkText() { return juce::Colour::fromRGB(38, 38, 38); }
juce::Colour creamText() { return juce::Colour::fromRGB(240, 219, 195); }
juce::Colour ledRed() { return juce::Colour::fromRGB(255, 50, 31); }
juce::Colour ledOff() { return juce::Colour::fromRGB(53, 5, 3); }
}

class Borato224AudioProcessorEditor::HardwareButtonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool isHighlighted, bool isDown) override
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

        const auto ledCentre = juce::Point<float>(r.getRight() - r.getWidth() * 0.22f, r.getBottom() - r.getHeight() * 0.25f);
        const auto ledRadius = juce::jlimit(3.5f, 6.5f, r.getHeight() * 0.10f);
        drawLed(g, ledCentre, on, ledRadius);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool isDown) override
    {
        auto r = button.getLocalBounds().toFloat().reduced(5.0f);
        r.translate(0.0f, isDown ? 2.0f : 0.0f);
        r.removeFromBottom(r.getHeight() * 0.18f);

        g.setColour(juce::Colour::fromRGB(220, 196, 135));
        g.setFont(juce::Font(juce::FontOptions(juce::jlimit(11.0f, 17.0f, r.getHeight() * 0.36f))).boldened());
        g.drawFittedText(button.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
    }

private:
    static void drawLed(juce::Graphics& g, juce::Point<float> c, bool on, float radius)
    {
        if (on)
        {
            g.setColour(ledRed().withAlpha(0.20f));
            g.fillEllipse(c.x - radius * 2.6f, c.y - radius * 2.6f, radius * 5.2f, radius * 5.2f);
            g.setColour(ledRed());
        }
        else
        {
            g.setColour(ledOff());
        }

        g.fillEllipse(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour((on ? juce::Colours::white : juce::Colour::fromRGB(140, 45, 38)).withAlpha(on ? 0.55f : 0.65f));
        g.drawEllipse(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    }
};

class Borato224AudioProcessorEditor::FaderLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float, float, const juce::Slider::SliderStyle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
        const float u = juce::jmax(0.45f, juce::jmin(bounds.getWidth() / 110.0f, bounds.getHeight() / 275.0f));
        auto slot = bounds.withWidth(16.0f * u).withCentre({ bounds.getCentreX(), bounds.getCentreY() }).reduced(0.0f, 21.0f * u);

        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(74, 74, 70), slot.getX(), slot.getCentreY(),
                                               juce::Colour::fromRGB(18, 18, 15), slot.getCentreX(), slot.getCentreY(), false));
        g.fillRoundedRectangle(slot, 8.0f * u);

        g.setColour(juce::Colours::black.withAlpha(0.65f));
        g.drawLine(slot.getCentreX(), slot.getY() + 8.0f * u, slot.getCentreX(), slot.getBottom() - 8.0f * u, 2.0f * u);

        for (int i = 0; i < 6; ++i)
        {
            const float ty = juce::jmap((float) i, 0.0f, 5.0f, slot.getY() + 8.0f * u, slot.getBottom() - 8.0f * u);
            g.setColour(juce::Colour::fromRGB(96, 90, 80));
            g.drawLine(slot.getX() - 22.0f * u, ty, slot.getX() - 8.0f * u, ty, 1.5f * u);
            g.drawLine(slot.getRight() + 8.0f * u, ty, slot.getRight() + 22.0f * u, ty, 1.5f * u);
        }

        const float capW = juce::jmin(84.0f * u, bounds.getWidth() - 4.0f * u);
        const float capH = 40.0f * u;
        const float capY = juce::jlimit(bounds.getY() + 2.0f * u, bounds.getBottom() - capH - 2.0f * u, sliderPos - capH * 0.5f);
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
};

Borato224AudioProcessorEditor::Borato224AudioProcessorEditor(Borato224AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setResizable(true, true);
    setResizeLimits(860, 645, 7680, 5760);
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio((double) designWidth / (double) designHeight);
    setSize(1200, 900);

    buttonLookAndFeel = std::make_unique<HardwareButtonLookAndFeel>();
    faderLookAndFeel = std::make_unique<FaderLookAndFeel>();

    configureProgramButtons();
    configureSystemButtons();
    configureFaders();
    updateProgramDisplay();
    refreshButtonStates();
    startTimerHz(24);
}

Borato224AudioProcessorEditor::~Borato224AudioProcessorEditor()
{
    stopTimer();

    for (auto& button : programButtons)
        button.setLookAndFeel(nullptr);
    for (auto& button : systemButtons)
        button.setLookAndFeel(nullptr);
    for (auto& fader : faders)
        fader.setLookAndFeel(nullptr);
}

void Borato224AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(32, 29, 24));
    drawPanel(g);
    drawDisplay(g, mapRect(182.0f, 256.0f, 1236.0f, 123.0f));
    drawDivider(g, 445.0f, "PROGRAM");
    drawDivider(g, 615.0f, "SYSTEM");
    drawFaderScales(g);
}

void Borato224AudioProcessorEditor::resized()
{
    constexpr std::array<float, 8> programX {{ 245, 395, 545, 695, 845, 995, 1145, 1295 }};
    for (size_t i = 0; i < programButtons.size(); ++i)
        programButtons[i].setBounds(mapRect(programX[i], 500.0f, 105.0f, 60.0f).toNearestInt());

    constexpr std::array<float, 8> systemX {{ 245, 395, 545, 695, 845, 995, 1145, 1295 }};
    for (size_t i = 0; i < systemButtons.size(); ++i)
        systemButtons[i].setBounds(mapRect(systemX[i], 650.0f, 120.0f, 62.0f).toNearestInt());

    constexpr std::array<float, 8> faderX {{ 185, 320, 455, 590, 725, 860, 995, 1235 }};
    for (size_t i = 0; i < faders.size(); ++i)
        faders[i].setBounds(mapRect(faderX[i], 825.0f, 110.0f, 275.0f).toNearestInt());
}

void Borato224AudioProcessorEditor::timerCallback()
{
    if (editMode)
        setDisplayForParameter(faderSpecs[(size_t) selectedParameter].parameterId);
    else
        updateProgramDisplay();

    refreshButtonStates();
    repaint();
}

void Borato224AudioProcessorEditor::configureProgramButtons()
{
    for (size_t i = 0; i < programButtons.size(); ++i)
    {
        auto& button = programButtons[i];
        button.setButtonText(programPresets[i].name);
        button.setClickingTogglesState(false);
        button.setLookAndFeel(buttonLookAndFeel.get());
        button.onClick = [this, i]
        {
            processor.applyProgramPreset((int) i);
            editMode = false;
            updateProgramDisplay();
            refreshButtonStates();
        };
        addAndMakeVisible(button);
    }
}

void Borato224AudioProcessorEditor::configureSystemButtons()
{
    for (size_t i = 0; i < systemButtons.size(); ++i)
    {
        auto& button = systemButtons[i];
        button.setButtonText(systemLabels[i]);
        button.setClickingTogglesState(false);
        button.setLookAndFeel(buttonLookAndFeel.get());
        addAndMakeVisible(button);
    }

    systemButtons[0].onClick = [this]
    {
        processor.storeSnapshot();
        storeFlash = true;
        display = { "SYSTEM", "STORE", "SAVED", {}, false, true };
        repaint();
    };

    systemButtons[1].onClick = [this]
    {
        processor.recallSnapshot();
        display = { "SYSTEM", "RECALL", "LOAD", {}, false, true };
        repaint();
    };

    systemButtons[2].onClick = [this]
    {
        processor.swapAB();
        display = { "SYSTEM", "A/B", processor.getABSlot() ? "B" : "A", {}, false, false };
        repaint();
    };

    systemButtons[3].onClick = [this]
    {
        compareHeld = ! compareHeld;
        if (compareHeld)
            processor.beginCompare();
        else
            processor.endCompare();

        display = { "SYSTEM", "COMPARE", compareHeld ? "ON" : "OFF", {}, false, false };
        repaint();
    };

    systemButtons[4].onClick = [this]
    {
        editMode = ! editMode;
        if (editMode)
            setDisplayForParameter(faderSpecs[(size_t) selectedParameter].parameterId);
        else
            updateProgramDisplay();
        repaint();
    };

    systemButtons[5].onClick = [this] { toggleBypassFromUi(); };
    systemButtons[6].onClick = [this] { nudgeSelectedParameter(-1.0f); };
    systemButtons[7].onClick = [this] { nudgeSelectedParameter(1.0f); };
}

void Borato224AudioProcessorEditor::configureFaders()
{
    for (size_t i = 0; i < faders.size(); ++i)
    {
        auto& fader = faders[i];
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader.setLookAndFeel(faderLookAndFeel.get());
        fader.setVelocityBasedMode(true);
        fader.setVelocityModeParameters(0.65, 1, 0.08, true);
        fader.setMouseDragSensitivity(260);
        if (auto* parameter = processor.apvts.getParameter(faderSpecs[i].parameterId))
            fader.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

        fader.onDragStart = [this, i]
        {
            selectedParameter = (int) i;
            editMode = true;
            setDisplayForParameter(faderSpecs[i].parameterId);
        };

        fader.onValueChange = [this, i]
        {
            selectedParameter = (int) i;
            if (editMode)
                setDisplayForParameter(faderSpecs[i].parameterId);
        };

        addAndMakeVisible(fader);
        faderAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, faderSpecs[i].parameterId, fader);
    }
}

void Borato224AudioProcessorEditor::refreshButtonStates()
{
    const int program = juce::jlimit(0, (int) programButtons.size() - 1,
                                    (int) processor.apvts.getRawParameterValue(ParamIDs::program)->load());

    for (size_t i = 0; i < programButtons.size(); ++i)
        programButtons[i].setToggleState((int) i == program, juce::dontSendNotification);

    systemButtons[2].setToggleState(processor.getABSlot(), juce::dontSendNotification);
    systemButtons[3].setToggleState(compareHeld, juce::dontSendNotification);
    systemButtons[4].setToggleState(editMode, juce::dontSendNotification);
    systemButtons[5].setToggleState(processor.apvts.getRawParameterValue(ParamIDs::bypass)->load() > 0.5f,
                                    juce::dontSendNotification);
    systemButtons[0].setToggleState(storeFlash, juce::dontSendNotification);
}

void Borato224AudioProcessorEditor::setDisplayForParameter(const juce::String& id)
{
    if (auto* value = processor.apvts.getRawParameterValue(id))
    {
        display.mode = "PARAMETER";
        display.label = displayLabelFor(id);
        display.value = formatValue(id, value->load());
        display.unit = unitFor(id);
        display.edit = true;
        display.stored = false;
    }
}

void Borato224AudioProcessorEditor::updateProgramDisplay()
{
    const int program = juce::jlimit(0, (int) programPresets.size() - 1,
                                    (int) processor.apvts.getRawParameterValue(ParamIDs::program)->load());
    display.mode = "MODE";
    display.label = "PROGRAM";
    display.value = programPresets[(size_t) program].name;
    display.unit = {};
    display.edit = false;
    display.stored = false;
}

void Borato224AudioProcessorEditor::nudgeSelectedParameter(float direction)
{
    auto* parameter = processor.apvts.getParameter(faderSpecs[(size_t) selectedParameter].parameterId);
    if (parameter == nullptr)
        return;

    const juce::String id = faderSpecs[(size_t) selectedParameter].parameterId;
    float plainStep = 0.5f;
    if (id == ParamIDs::decay)
        plainStep = 0.10f;
    else if (id == ParamIDs::preDelay)
        plainStep = 2.0f;
    else if (id == ParamIDs::crossover)
        plainStep = 25.0f;
    else if (id == ParamIDs::mix)
        plainStep = 1.0f;

    const float currentPlain = parameter->convertFrom0to1(parameter->getValue());
    const float targetPlain = currentPlain + plainStep * direction;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, parameter->convertTo0to1(targetPlain)));
    parameter->endChangeGesture();
    editMode = true;
    setDisplayForParameter(faderSpecs[(size_t) selectedParameter].parameterId);
}

void Borato224AudioProcessorEditor::toggleBypassFromUi()
{
    if (auto* param = processor.apvts.getParameter(ParamIDs::bypass))
    {
        const bool bypass = processor.apvts.getRawParameterValue(ParamIDs::bypass)->load() < 0.5f;
        param->beginChangeGesture();
        param->setValueNotifyingHost(bypass ? 1.0f : 0.0f);
        param->endChangeGesture();
        display = { "SYSTEM", "BYPASS", bypass ? "ON" : "OFF", {}, false, false };
    }
}

juce::Rectangle<float> Borato224AudioProcessorEditor::canvasBounds() const
{
    auto area = getLocalBounds().toFloat().reduced(8.0f);
    const float scale = std::min(area.getWidth() / designWidth, area.getHeight() / designHeight);
    const auto w = designWidth * scale;
    const auto h = designHeight * scale;
    return { area.getCentreX() - w * 0.5f, area.getCentreY() - h * 0.5f, w, h };
}

juce::Rectangle<float> Borato224AudioProcessorEditor::mapRect(float x, float y, float w, float h) const
{
    const auto canvas = canvasBounds();
    const auto sx = canvas.getWidth() / designWidth;
    const auto sy = canvas.getHeight() / designHeight;
    return { canvas.getX() + x * sx, canvas.getY() + y * sy, w * sx, h * sy };
}

juce::Point<float> Borato224AudioProcessorEditor::mapPoint(float x, float y) const
{
    const auto canvas = canvasBounds();
    const auto sx = canvas.getWidth() / designWidth;
    const auto sy = canvas.getHeight() / designHeight;
    return { canvas.getX() + x * sx, canvas.getY() + y * sy };
}

void Borato224AudioProcessorEditor::drawPanel(juce::Graphics& g)
{
    const auto rack = mapRect(panelLeft, panelTop, panelWidth, panelHeight);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(rack.translated(0.0f, rack.getHeight() * 0.01f), 14.0f);

    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(247, 242, 230), rack.getX(), rack.getY(),
                                           juce::Colour::fromRGB(211, 202, 184), rack.getX(), rack.getBottom(), false));
    g.fillRoundedRectangle(rack, 14.0f);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.fillRoundedRectangle(rack.reduced(8.0f).removeFromTop(rack.getHeight() * 0.16f), 9.0f);
    g.setColour(juce::Colour::fromRGB(174, 166, 151));
    g.drawRoundedRectangle(rack.reduced(0.6f), 14.0f, 1.5f);

    const auto leftEar = mapRect(25.0f, panelTop, 72.0f, panelHeight);
    const auto rightEar = mapRect(1503.0f, panelTop, 72.0f, panelHeight);
    for (auto ear : { leftEar, rightEar })
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(216, 208, 192), ear.getX(), ear.getY(),
                                               juce::Colour::fromRGB(200, 191, 173), ear.getRight(), ear.getY(), false));
        g.fillRoundedRectangle(ear, 8.0f);
        g.setColour(juce::Colour::fromRGB(184, 175, 158));
        g.drawRoundedRectangle(ear.reduced(0.5f), 8.0f, 1.0f);
    }

    drawPcb(g, mapRect(185.0f, 35.0f, 1230.0f, 80.0f));

    drawScrew(g, mapPoint(125.0f, 145.0f), mapRect(0, 0, 15, 15).getWidth());
    drawScrew(g, mapPoint(1475.0f, 145.0f), mapRect(0, 0, 15, 15).getWidth());
    drawScrew(g, mapPoint(125.0f, 1150.0f), mapRect(0, 0, 15, 15).getWidth());
    drawScrew(g, mapPoint(1475.0f, 1150.0f), mapRect(0, 0, 15, 15).getWidth());

    g.setColour(brass());
    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 46).getHeight())));
    g.drawText("Model 224", mapRect(170.0f, 122.0f, 320.0f, 58.0f), juce::Justification::centredLeft);
    g.setColour(darkText());
    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 23).getHeight())).withExtraKerningFactor(0.08f));
    g.drawText("DIGITAL REVERB SYSTEM", mapRect(172.0f, 178.0f, 430.0f, 32.0f), juce::Justification::centredLeft);

    g.setColour(brass());
    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 58).getHeight())).withTypefaceStyle("Bold Italic"));
    g.drawText("Borato", mapRect(1135.0f, 120.0f, 260.0f, 70.0f), juce::Justification::centred);
    g.setColour(darkText());
    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 22).getHeight())).withExtraKerningFactor(0.08f));
    g.drawText("STUDIO REVERBERATOR", mapRect(1115.0f, 178.0f, 360.0f, 32.0f), juce::Justification::centred);

    const auto faderBay = mapRect(180.0f, 755.0f, 1240.0f, 360.0f);
    g.setColour(juce::Colours::black.withAlpha(0.04f));
    g.fillRoundedRectangle(faderBay, 8.0f);
    g.setColour(brass());
    g.drawRoundedRectangle(faderBay, 8.0f, 1.5f);

    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 18).getHeight())));
    g.drawText("BORATO AUDIO INC.", mapRect(170.0f, 1138.0f, 260.0f, 26.0f), juce::Justification::centredLeft);
    g.drawText("MADE IN BRAZIL", mapRect(1160.0f, 1138.0f, 210.0f, 26.0f), juce::Justification::centredLeft);
    g.drawText("1978", mapRect(1375.0f, 1138.0f, 80.0f, 26.0f), juce::Justification::centredLeft);
}

void Borato224AudioProcessorEditor::drawPcb(juce::Graphics& g, juce::Rectangle<float> area)
{
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
    g.drawText("REV 224 LOGIC / MEMORY", juce::Rectangle<float> { x(0.015f), y(0.06f), s(2.1f), s(0.16f) }, juce::Justification::centredLeft);
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

void Borato224AudioProcessorEditor::drawScrew(juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour(juce::Colour::fromRGB(23, 23, 23));
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour::fromRGB(104, 104, 104));
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    g.drawLine(centre.x - radius * 0.55f, centre.y, centre.x + radius * 0.55f, centre.y, 2.0f);
}

void Borato224AudioProcessorEditor::drawDivider(juce::Graphics& g, float y, const juce::String& title)
{
    const auto left = mapPoint(245.0f, y);
    const auto right = mapPoint(1355.0f, y);
    const auto centre = mapPoint(800.0f, y);
    const auto gap = mapRect(0, 0, 170, 1).getWidth();

    g.setColour(brass());
    g.drawLine(left.x, left.y, centre.x - gap * 0.5f, centre.y, 1.6f);
    g.drawLine(centre.x + gap * 0.5f, centre.y, right.x, right.y, 1.6f);
    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 28).getHeight())).boldened());
    g.drawText(title, mapRect(700.0f, y - 26.0f, 200.0f, 40.0f), juce::Justification::centred);
}

void Borato224AudioProcessorEditor::drawDisplay(juce::Graphics& g, juce::Rectangle<float> area)
{
    auto outer = area.expanded(mapRect(0, 0, 12, 12).getWidth(), mapRect(0, 0, 11, 11).getHeight());
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
        display.unit == "sec",
        display.unit == "ms",
        display.unit == "Hz",
        display.unit == "dB"
    }};

    g.setColour(creamText());
    g.setFont(juce::Font(juce::FontOptions(16.0f * sy)));
    for (size_t i = 0; i < unitRows.size(); ++i)
    {
        drawLed(g, { px(48), py(unitRows[i]) }, unitActive[i], (i == 0 ? 6.4f : 5.7f) * sx);
        g.setColour(creamText().withAlpha(unitActive[i] ? 1.0f : 0.72f));
        g.drawText(unitLabels[i], juce::Rectangle<float>(px(77), py(unitRows[i] - 12.0f), 72 * sx, 23 * sy), juce::Justification::centredLeft);
    }

    g.setFont(juce::Font(juce::FontOptions(17.0f * sy)).withExtraKerningFactor(0.10f));
    g.setColour(creamText());
    g.drawText(display.mode, juce::Rectangle<float>(px(210), py(22), 230 * sx, 23 * sy), juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(34.0f * sy)).boldened());
    g.drawFittedText(display.label, juce::Rectangle<float>(px(210), py(48), 270 * sx, 48 * sy).toNearestInt(),
                     juce::Justification::centredLeft, 1);

    g.setColour(ledRed().withAlpha(0.14f));
    g.setFont(juce::Font(juce::FontOptions(70.0f * sy)).withTypefaceStyle("Bold"));
    g.drawText("888888", juce::Rectangle<float>(px(492), py(20), 450 * sx, 84 * sy), juce::Justification::centredLeft);

    g.setColour(ledRed());
    g.drawText(display.value, juce::Rectangle<float>(px(492), py(20), 450 * sx, 84 * sy), juce::Justification::centredLeft);

    constexpr std::array<float, 3> statusRows {{ 31.0f, 66.0f, 101.0f }};
    constexpr std::array<const char*, 3> statusLabels {{ "Program", "Edit", "Store" }};
    const std::array<bool, 3> statusActive {{ ! display.edit, display.edit, display.stored }};
    g.setFont(juce::Font(juce::FontOptions(16.5f * sy)));
    for (size_t i = 0; i < statusRows.size(); ++i)
    {
        drawLed(g, { px(1098), py(statusRows[i]) }, statusActive[i], (i == 1 ? 6.3f : 5.8f) * sx);
        g.setColour(creamText().withAlpha(statusActive[i] ? 1.0f : 0.72f));
        g.drawText(statusLabels[i], juce::Rectangle<float>(px(1128), py(statusRows[i] - 12.0f), 100 * sx, 23 * sy),
                   juce::Justification::centredLeft);
    }
}

void Borato224AudioProcessorEditor::drawLed(juce::Graphics& g, juce::Point<float> centre, bool on, float radius)
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

void Borato224AudioProcessorEditor::drawFaderScales(juce::Graphics& g)
{
    constexpr std::array<float, 8> centreX {{ 240, 375, 510, 645, 780, 915, 1050, 1290 }};

    for (size_t i = 0; i < faderSpecs.size(); ++i)
    {
        const auto& spec = faderSpecs[i];
        auto title = mapRect(centreX[i] - 86.0f, 782.0f, 172.0f, 36.0f);
        g.setColour(brass());
        const bool twoLineTitle = juce::String(spec.title) == "TREBLE DECAY";
        g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, twoLineTitle ? 18.0f : 22.0f).getHeight())).boldened());
        g.drawFittedText(spec.title, title.toNearestInt(), juce::Justification::centred, 2);

        const auto labelW = mapRect(0.0f, 0.0f, 48.0f, 1.0f).getWidth();
        const auto labelH = mapRect(0.0f, 0.0f, 1.0f, 22.0f).getHeight();
        const auto labelX = mapPoint(centreX[i] - 70.0f, 0.0f).x;
        const auto topY = mapPoint(0.0f, 872.0f).y - labelH * 0.5f;
        const auto midY = mapPoint(0.0f, 962.0f).y - labelH * 0.5f;
        const auto bottomY = mapPoint(0.0f, 1052.0f).y - labelH * 0.5f;

        g.setColour(darkText().withAlpha(0.82f));
        g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 16).getHeight())));
        g.drawText(spec.top, juce::Rectangle<float>(labelX, topY, labelW, labelH), juce::Justification::centredRight);
        g.drawText(spec.mid, juce::Rectangle<float>(labelX, midY, labelW, labelH), juce::Justification::centredRight);
        g.drawText(spec.bottom, juce::Rectangle<float>(labelX, bottomY, labelW, labelH), juce::Justification::centredRight);

        g.setColour(darkText().withAlpha(0.75f));
        g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 15).getHeight())));
        g.drawText(spec.unit, mapRect(centreX[i] - 28.0f, 1106.0f, 56.0f, 28.0f), juce::Justification::centred);
    }
}

juce::String Borato224AudioProcessorEditor::formatValue(const juce::String& id, float value)
{
    if (id == ParamIDs::decay) return juce::String(value, 2);
    if (id == ParamIDs::preDelay) return juce::String((int) std::round(value)).paddedLeft('0', 3);
    if (id == ParamIDs::crossover) return juce::String((int) std::round(value));
    if (id == ParamIDs::mix) return juce::String(value, 1);
    return juce::String(value, 1, false);
}

juce::String Borato224AudioProcessorEditor::displayLabelFor(const juce::String& id)
{
    if (id == ParamIDs::decay) return "DECAY";
    if (id == ParamIDs::preDelay) return "PRE";
    if (id == ParamIDs::bass) return "BASS";
    if (id == ParamIDs::mid) return "MID";
    if (id == ParamIDs::trebleDecay) return "TREBLE";
    if (id == ParamIDs::crossover) return "XOVER";
    if (id == ParamIDs::depth) return "DEPTH";
    if (id == ParamIDs::mix) return "MIX";
    if (id == ParamIDs::input) return "INPUT";
    if (id == ParamIDs::output) return "OUTPUT";
    return "PARAM";
}

juce::String Borato224AudioProcessorEditor::unitFor(const juce::String& id)
{
    if (id == ParamIDs::decay) return "sec";
    if (id == ParamIDs::preDelay) return "ms";
    if (id == ParamIDs::crossover) return "Hz";
    if (id == ParamIDs::bass || id == ParamIDs::mid || id == ParamIDs::trebleDecay || id == ParamIDs::depth
        || id == ParamIDs::input || id == ParamIDs::output) return "dB";
    if (id == ParamIDs::mix) return "%";
    return {};
}
