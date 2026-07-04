#include "PluginEditor.h"
#include "PluginParameters.h"

using namespace Borato224Colours;

Borato224AudioProcessorEditor::Borato224AudioProcessorEditor(Borato224AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      programButtons(processor.apvts, lookAndFeel),
      systemButtons(lookAndFeel),
      faderBay(processor.apvts, lookAndFeel)
{
    setResizable(true, true);
    setResizeLimits(860, 645, 7680, 5760);
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio((double) designWidth / (double) designHeight);
    setSize(1200, 900);

    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(pcb);
    addAndMakeVisible(displayComponent);
    addAndMakeVisible(programButtons);
    addAndMakeVisible(systemButtons);
    addAndMakeVisible(faderBay);

    programButtons.setOnProgramSelected([this](int i)
    {
        endCompareIfActive();
        processor.applyProgramPreset(i);
        editMode = false;
        updateProgramDisplay();
    });

    systemButtons.setCallbacks({
        [this]
        {
            endCompareIfActive();
            processor.storeSnapshot();
            storeFlashTicks = statusHoldTicks;
            showSystemMessage("STORE", "SAVED", true);
        },
        [this]
        {
            endCompareIfActive();
            processor.recallSnapshot();
            showSystemMessage("RECALL", "LOADED");
        },
        [this]
        {
            endCompareIfActive();
            processor.swapAB();
            showSystemMessage("A/B", processor.getABSlot() ? "B" : "A");
        },
        [this]
        {
            if (compareHeld)
            {
                processor.endCompare();
                compareHeld = false;
            }
            else
            {
                processor.beginCompare();
                compareHeld = processor.isComparing();
            }
            showSystemMessage("COMPARE", compareHeld ? "ON" : "OFF");
        },
        [this]
        {
            editMode = ! editMode;
            if (editMode)
                updateDisplayForParameter(FaderBayComponent::specs[(size_t) selectedParameter].parameterId);
            else
                updateProgramDisplay();
            repaint();
        },
        [this] { toggleBypassFromUi(); },
        [this] { nudgeSelectedParameter(-1.0f); },
        [this] { nudgeSelectedParameter(1.0f); }
    });

    faderBay.setOnFaderSelected([this](int i)
    {
        endCompareIfActive();
        selectedParameter = i;
        editMode = true;
        updateDisplayForParameter(FaderBayComponent::specs[(size_t) i].parameterId);
    });

    updateProgramDisplay();
    startTimerHz(24);
}

Borato224AudioProcessorEditor::~Borato224AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void Borato224AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(32, 29, 24));
    drawPanel(g);

    const int program = juce::jlimit(0, 7, (int) processor.apvts.getRawParameterValue(ParamIDs::program)->load());
    const bool bypassOn = processor.apvts.getRawParameterValue(ParamIDs::bypass)->load() > 0.5f;
    programButtons.setActiveProgram(program);
    systemButtons.setButtonStates(processor.getABSlot(), compareHeld, editMode, bypassOn, storeFlashTicks > 0);
}

void Borato224AudioProcessorEditor::resized()
{
    pcb.setBounds(mapRect(185.0f, 35.0f, 1230.0f, 80.0f).toNearestInt());
    displayComponent.setBounds(mapRect(170.0f, 245.0f, 1260.0f, 145.0f).toNearestInt());

    const auto canvas = canvasBounds().toNearestInt();
    programButtons.setBounds(canvas);
    systemButtons.setBounds(canvas);
    faderBay.setBounds(canvas);
}

void Borato224AudioProcessorEditor::timerCallback()
{
    compareHeld = processor.isComparing();

    if (transientDisplayTicks > 0)
        --transientDisplayTicks;
    else if (editMode)
        updateDisplayForParameter(FaderBayComponent::specs[(size_t) selectedParameter].parameterId);
    else
        updateProgramDisplay();

    if (storeFlashTicks > 0)
        --storeFlashTicks;

    const int program = juce::jlimit(0, 7, (int) processor.apvts.getRawParameterValue(ParamIDs::program)->load());
    const bool bypassOn = processor.apvts.getRawParameterValue(ParamIDs::bypass)->load() > 0.5f;
    programButtons.setActiveProgram(program);
    systemButtons.setButtonStates(processor.getABSlot(), compareHeld, editMode, bypassOn, storeFlashTicks > 0);
    repaint();
}

void Borato224AudioProcessorEditor::updateDisplayForParameter(const juce::String& id)
{
    if (auto* value = processor.apvts.getRawParameterValue(id))
    {
        display.mode = "PARAMETER";
        display.label = displayLabelFor(id);
        display.value = formatValue(id, value->load());
        display.unit = unitFor(id);
        display.edit = true;
        display.stored = false;
        displayComponent.setState(display);
    }
}

void Borato224AudioProcessorEditor::updateProgramDisplay()
{
    const int program = juce::jlimit(0, 7, (int) processor.apvts.getRawParameterValue(ParamIDs::program)->load());
    display.mode = "MODE";
    display.label = "PROGRAM";
    display.value = programPresets[(size_t) program].name;
    display.unit = {};
    display.edit = false;
    display.stored = false;
    displayComponent.setState(display);
}

void Borato224AudioProcessorEditor::showSystemMessage(const juce::String& label,
                                                       const juce::String& value,
                                                       bool stored)
{
    display = { "SYSTEM", label, value, {}, false, stored };
    displayComponent.setState(display);
    transientDisplayTicks = statusHoldTicks;
    repaint();
}

void Borato224AudioProcessorEditor::endCompareIfActive()
{
    if (! compareHeld && ! processor.isComparing())
        return;

    processor.endCompare();
    compareHeld = false;
}

void Borato224AudioProcessorEditor::nudgeSelectedParameter(float direction)
{
    endCompareIfActive();

    auto* parameter = processor.apvts.getParameter(FaderBayComponent::specs[(size_t) selectedParameter].parameterId);
    if (parameter == nullptr)
        return;

    const juce::String id = FaderBayComponent::specs[(size_t) selectedParameter].parameterId;
    float plainStep = 0.5f;
    if (id == ParamIDs::decay)       plainStep = 0.10f;
    else if (id == ParamIDs::preDelay)   plainStep = 2.0f;
    else if (id == ParamIDs::crossover)  plainStep = 25.0f;
    else if (id == ParamIDs::mix)        plainStep = 1.0f;

    const float currentPlain = parameter->convertFrom0to1(parameter->getValue());
    const float targetPlain = currentPlain + plainStep * direction;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, parameter->convertTo0to1(targetPlain)));
    parameter->endChangeGesture();
    editMode = true;
    updateDisplayForParameter(FaderBayComponent::specs[(size_t) selectedParameter].parameterId);
}

void Borato224AudioProcessorEditor::toggleBypassFromUi()
{
    endCompareIfActive();

    if (auto* param = processor.apvts.getParameter(ParamIDs::bypass))
    {
        const bool bypass = processor.apvts.getRawParameterValue(ParamIDs::bypass)->load() < 0.5f;
        param->beginChangeGesture();
        param->setValueNotifyingHost(bypass ? 1.0f : 0.0f);
        param->endChangeGesture();
        showSystemMessage("BYPASS", bypass ? "ON" : "OFF");
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
    const auto rack = mapRect(90.0f, 110.0f, 1420.0f, 1075.0f);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(rack.translated(0.0f, rack.getHeight() * 0.01f), 14.0f);

    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(247, 242, 230), rack.getX(), rack.getY(),
                                           juce::Colour::fromRGB(211, 202, 184), rack.getX(), rack.getBottom(), false));
    g.fillRoundedRectangle(rack, 14.0f);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.fillRoundedRectangle(rack.reduced(8.0f).removeFromTop(rack.getHeight() * 0.16f), 9.0f);
    g.setColour(juce::Colour::fromRGB(174, 166, 151));
    g.drawRoundedRectangle(rack.reduced(0.6f), 14.0f, 1.5f);

    const auto leftEar = mapRect(25.0f, 110.0f, 72.0f, 1075.0f);
    const auto rightEar = mapRect(1503.0f, 110.0f, 72.0f, 1075.0f);
    for (auto ear : { leftEar, rightEar })
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(216, 208, 192), ear.getX(), ear.getY(),
                                               juce::Colour::fromRGB(200, 191, 173), ear.getRight(), ear.getY(), false));
        g.fillRoundedRectangle(ear, 8.0f);
        g.setColour(juce::Colour::fromRGB(184, 175, 158));
        g.drawRoundedRectangle(ear.reduced(0.5f), 8.0f, 1.0f);
    }

    Borato224LookAndFeel::drawScrew(g, mapPoint(125.0f, 145.0f), mapRect(0, 0, 15, 15).getWidth());
    Borato224LookAndFeel::drawScrew(g, mapPoint(1475.0f, 145.0f), mapRect(0, 0, 15, 15).getWidth());
    Borato224LookAndFeel::drawScrew(g, mapPoint(125.0f, 1150.0f), mapRect(0, 0, 15, 15).getWidth());
    Borato224LookAndFeel::drawScrew(g, mapPoint(1475.0f, 1150.0f), mapRect(0, 0, 15, 15).getWidth());

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

    const auto faderBayArea = mapRect(180.0f, 755.0f, 1240.0f, 360.0f);
    g.setColour(juce::Colours::black.withAlpha(0.04f));
    g.fillRoundedRectangle(faderBayArea, 8.0f);
    g.setColour(brass());
    g.drawRoundedRectangle(faderBayArea, 8.0f, 1.5f);

    g.setFont(juce::Font(juce::FontOptions(mapRect(0, 0, 0, 18).getHeight())));
    g.drawText("BORATO COMPANY", mapRect(170.0f, 1138.0f, 260.0f, 26.0f), juce::Justification::centredLeft);
    g.drawText("MADE IN BRAZIL", mapRect(1160.0f, 1138.0f, 210.0f, 26.0f), juce::Justification::centredLeft);
    g.drawText("1978", mapRect(1375.0f, 1138.0f, 80.0f, 26.0f), juce::Justification::centredLeft);

    Borato224LookAndFeel::drawDivider(g, mapPoint(245.0f, 445.0f), mapPoint(1355.0f, 445.0f),
                                      mapPoint(800.0f, 445.0f), mapRect(0, 0, 170, 1).getWidth(),
                                      "PROGRAM", mapRect(0, 0, 0, 28).getHeight());
    Borato224LookAndFeel::drawDivider(g, mapPoint(245.0f, 615.0f), mapPoint(1355.0f, 615.0f),
                                      mapPoint(800.0f, 615.0f), mapRect(0, 0, 170, 1).getWidth(),
                                      "SYSTEM", mapRect(0, 0, 0, 28).getHeight());
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
