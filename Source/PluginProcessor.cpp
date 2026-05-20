#include "PluginProcessor.h"
#include "PluginEditor.h"

Borato224AudioProcessor::Borato224AudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    decayParam = apvts.getRawParameterValue(ParamIDs::decay);
    preDelayParam = apvts.getRawParameterValue(ParamIDs::preDelay);
    bassParam = apvts.getRawParameterValue(ParamIDs::bass);
    midParam = apvts.getRawParameterValue(ParamIDs::mid);
    trebleParam = apvts.getRawParameterValue(ParamIDs::trebleDecay);
    crossoverParam = apvts.getRawParameterValue(ParamIDs::crossover);
    depthParam = apvts.getRawParameterValue(ParamIDs::depth);
    mixParam = apvts.getRawParameterValue(ParamIDs::mix);
    inputParam = apvts.getRawParameterValue(ParamIDs::input);
    outputParam = apvts.getRawParameterValue(ParamIDs::output);
    bypassParam = apvts.getRawParameterValue(ParamIDs::bypass);

    storedSnapshot = captureEditableParameters(apvts);
    abA = storedSnapshot;
    abB = storedSnapshot;
}

void Borato224AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    reverb.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, false, true);

    mixSmooth.reset(sampleRate, 0.025);
    inputSmooth.reset(sampleRate, 0.010);
    outputSmooth.reset(sampleRate, 0.010);
    bypassSmooth.reset(sampleRate, 0.012);

    mixSmooth.setCurrentAndTargetValue(mixParam->load() / 100.0f);
    inputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputParam->load()));
    outputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputParam->load()));
    bypassSmooth.setCurrentAndTargetValue(bypassParam->load() > 0.5f ? 1.0f : 0.0f);
}

void Borato224AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();

    for (int ch = inputChannels; ch < outputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    wetBuffer.setSize(outputChannels, numSamples, false, false, true);
    wetBuffer.makeCopyOf(buffer, true);

    inputSmooth.setTargetValue(juce::Decibels::decibelsToGain(inputParam->load()));
    outputSmooth.setTargetValue(juce::Decibels::decibelsToGain(outputParam->load()));
    mixSmooth.setTargetValue(mixParam->load() / 100.0f);
    bypassSmooth.setTargetValue(bypassParam->load() > 0.5f ? 1.0f : 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inGain = inputSmooth.getNextValue();
        for (int ch = 0; ch < outputChannels; ++ch)
        {
            auto* wet = wetBuffer.getWritePointer(ch);
            wet[i] *= inGain;
        }
    }

    reverb.process(wetBuffer, (int) apvts.getRawParameterValue(ParamIDs::program)->load(),
                   decayParam->load(), preDelayParam->load(), bassParam->load(), midParam->load(),
                   trebleParam->load(), crossoverParam->load(), depthParam->load());

    // Compensate for the internal attenuation of the FDN (injection ~0.21, output taps ~0.33, vintageGain ~0.78).
    // This brings the wet signal to a perceptually comparable level with the dry signal at mix=50%.
    static constexpr float wetMakeupGain = 2.2f; // approximately +6.8 dB
    wetBuffer.applyGain(wetMakeupGain);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.getNextValue();
        const float mixAngle = juce::jlimit(0.0f, 1.0f, mix) * juce::MathConstants<float>::halfPi;
        const float dryMixGain = std::cos(mixAngle);
        const float wetMixGain = std::sin(mixAngle);
        const float bypass = bypassSmooth.getNextValue();
        const float outGain = outputSmooth.getNextValue();

        for (int ch = 0; ch < outputChannels; ++ch)
        {
            auto* dry = buffer.getWritePointer(ch);
            auto* wet = wetBuffer.getReadPointer(ch);
            const float effected = dry[i] * dryMixGain + wet[i] * wetMixGain;
            dry[i] = (effected * (1.0f - bypass) + dry[i] * bypass) * outGain;
        }
    }
}

juce::AudioProcessorEditor* Borato224AudioProcessor::createEditor()
{
    return new Borato224AudioProcessorEditor(*this);
}

int Borato224AudioProcessor::getCurrentProgram()
{
    return (int) apvts.getRawParameterValue(ParamIDs::program)->load();
}

void Borato224AudioProcessor::setCurrentProgram(int index)
{
    applyProgramPreset(index);
}

const juce::String Borato224AudioProcessor::getProgramName(int index)
{
    if ((size_t) index < programPresets.size())
        return programPresets[(size_t) index].name;
    return {};
}

void Borato224AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = createPersistedStateXml(apvts, storedSnapshot, compareSnapshot, abA, abB, abSlotB, comparing))
        copyXmlToBinary(*xml, destData);
}

void Borato224AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("BORATO224_STATE"))
            restorePersistedState(*xml);
        else
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

bool Borato224AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo();
}

void Borato224AudioProcessor::applyProgramPreset(int index)
{
    if ((size_t) index >= programPresets.size())
        return;

    const auto& p = programPresets[(size_t) index];
    setParameterValue(apvts, ParamIDs::program, (float) index);

    if (index == 6)
    {
        setParameterValue(apvts, ParamIDs::decay, randomRange(randomPresetRng, 1.20f, 8.80f));
        setParameterValue(apvts, ParamIDs::preDelay, randomRange(randomPresetRng, 8.0f, 115.0f));
        setParameterValue(apvts, ParamIDs::bass, randomRange(randomPresetRng, -4.5f, 4.5f));
        setParameterValue(apvts, ParamIDs::mid, randomRange(randomPresetRng, -3.5f, 3.5f));
        setParameterValue(apvts, ParamIDs::trebleDecay, randomRange(randomPresetRng, -7.5f, 2.5f));
        setParameterValue(apvts, ParamIDs::crossover, randomRange(randomPresetRng, 180.0f, 1450.0f));
        setParameterValue(apvts, ParamIDs::depth, randomRange(randomPresetRng, 1.5f, 10.0f));
        setParameterValue(apvts, ParamIDs::mix, randomRange(randomPresetRng, 22.0f, 42.0f));
        return;
    }

    setParameterValue(apvts, ParamIDs::decay, p.decay);
    setParameterValue(apvts, ParamIDs::preDelay, p.preDelayMs);
    setParameterValue(apvts, ParamIDs::bass, p.bassDb);
    setParameterValue(apvts, ParamIDs::mid, p.midDb);
    setParameterValue(apvts, ParamIDs::trebleDecay, p.trebleDecayDb);
    setParameterValue(apvts, ParamIDs::crossover, p.crossoverHz);
    setParameterValue(apvts, ParamIDs::depth, p.depthDb);
    setParameterValue(apvts, ParamIDs::mix, p.mixPercent);
}

void Borato224AudioProcessor::storeSnapshot()
{
    storedSnapshot = captureEditableParameters(apvts);
}

void Borato224AudioProcessor::recallSnapshot()
{
    applyParameterMap(apvts, storedSnapshot);
}

void Borato224AudioProcessor::swapAB()
{
    auto current = captureEditableParameters(apvts);
    if (abSlotB)
    {
        abB = current;
        applyParameterMap(apvts, abA);
    }
    else
    {
        abA = current;
        applyParameterMap(apvts, abB);
    }
    abSlotB = ! abSlotB;
}

void Borato224AudioProcessor::beginCompare()
{
    if (comparing)
        return;
    compareSnapshot = captureEditableParameters(apvts);
    applyParameterMap(apvts, storedSnapshot);
    comparing = true;
}

void Borato224AudioProcessor::endCompare()
{
    if (! comparing)
        return;
    applyParameterMap(apvts, compareSnapshot);
    comparing = false;
}

juce::XmlElement Borato224AudioProcessor::createParameterMapXml(const juce::String& tagName, const ParameterMap& map)
{
    juce::XmlElement element(tagName);
    for (const auto& [id, value] : map)
    {
        auto* param = element.createNewChildElement("PARAM");
        param->setAttribute("id", id);
        param->setAttribute("value", value);
    }
    return element;
}

Borato224AudioProcessor::ParameterMap Borato224AudioProcessor::parseParameterMapXml(const juce::XmlElement& element)
{
    ParameterMap map;
    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->hasTagName("PARAM"))
        {
            const auto id = child->getStringAttribute("id");
            if (id.isNotEmpty())
                map[id] = child->getDoubleAttribute("value", 0.0);
        }
    }
    return map;
}

std::unique_ptr<juce::XmlElement> Borato224AudioProcessor::createPersistedStateXml(
    const juce::AudioProcessorValueTreeState& state,
    const ParameterMap& storedSnapshotIn,
    const ParameterMap& compareSnapshotIn,
    const ParameterMap& abAIn,
    const ParameterMap& abBIn,
    bool abSlotBIn,
    bool comparingIn)
{
    auto root = std::make_unique<juce::XmlElement>("BORATO224_STATE");
    root->setAttribute("version", 1);
    root->setAttribute("abSlotB", abSlotBIn ? 1 : 0);
    root->setAttribute("comparing", comparingIn ? 1 : 0);

    root->addChildElement(state.state.createXml().release());
    root->addChildElement(new juce::XmlElement(createParameterMapXml("STORED_SNAPSHOT", storedSnapshotIn)));
    root->addChildElement(new juce::XmlElement(createParameterMapXml("COMPARE_SNAPSHOT", compareSnapshotIn)));
    root->addChildElement(new juce::XmlElement(createParameterMapXml("AB_A", abAIn)));
    root->addChildElement(new juce::XmlElement(createParameterMapXml("AB_B", abBIn)));
    return root;
}

void Borato224AudioProcessor::restorePersistedState(const juce::XmlElement& element)
{
    if (auto* child = element.getFirstChildElement())
    {
        while (child != nullptr)
        {
            const auto tag = child->getTagName();
            if (tag != "STORED_SNAPSHOT" && tag != "COMPARE_SNAPSHOT" && tag != "AB_A" && tag != "AB_B")
            {
                apvts.replaceState(juce::ValueTree::fromXml(*child));
                break;
            }

            child = child->getNextElement();
        }
    }

    if (auto* stored = element.getChildByName("STORED_SNAPSHOT"))
        storedSnapshot = parseParameterMapXml(*stored);
    else
        storedSnapshot = captureEditableParameters(apvts);

    if (auto* compare = element.getChildByName("COMPARE_SNAPSHOT"))
        compareSnapshot = parseParameterMapXml(*compare);
    else
        compareSnapshot.clear();

    if (auto* a = element.getChildByName("AB_A"))
        abA = parseParameterMapXml(*a);
    else
        abA = storedSnapshot;

    if (auto* b = element.getChildByName("AB_B"))
        abB = parseParameterMapXml(*b);
    else
        abB = storedSnapshot;

    abSlotB = element.getBoolAttribute("abSlotB", false);
    comparing = element.getBoolAttribute("comparing", false);
}

void Borato224AudioProcessor::setParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value)
{
    if (auto* parameter = state.getParameter(id))
    {
        const float normalised = parameter->convertTo0to1(value);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(normalised);
        parameter->endChangeGesture();
    }
}

float Borato224AudioProcessor::randomRange(juce::Random& rng, float min, float max)
{
    return min + rng.nextFloat() * (max - min);
}

Borato224AudioProcessor::ParameterMap Borato224AudioProcessor::captureEditableParameters(const juce::AudioProcessorValueTreeState& state)
{
    ParameterMap map;
    for (const auto* id : { ParamIDs::program, ParamIDs::decay, ParamIDs::preDelay, ParamIDs::bass,
                            ParamIDs::mid, ParamIDs::trebleDecay, ParamIDs::crossover, ParamIDs::depth,
                            ParamIDs::mix, ParamIDs::input, ParamIDs::output, ParamIDs::bypass })
    {
        if (auto* value = state.getRawParameterValue(id))
            map[id] = value->load();
    }
    return map;
}

void Borato224AudioProcessor::applyParameterMap(juce::AudioProcessorValueTreeState& state, const ParameterMap& map)
{
    for (const auto& [id, value] : map)
        setParameterValue(state, id, value);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Borato224AudioProcessor();
}
