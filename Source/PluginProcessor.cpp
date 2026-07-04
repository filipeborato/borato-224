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

    programParam = apvts.getRawParameterValue(ParamIDs::program);
    mixParam = apvts.getRawParameterValue(ParamIDs::mix);
    inputParam = apvts.getRawParameterValue(ParamIDs::input);
    outputParam = apvts.getRawParameterValue(ParamIDs::output);
    bypassParam = apvts.getRawParameterValue(ParamIDs::bypass);

    customUserPreset = programPresets[7];

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

    if (wetBuffer.getNumChannels() != outputChannels || wetBuffer.getNumSamples() != numSamples)
        wetBuffer.setSize(outputChannels, numSamples, false, false, true);

    for (int ch = 0; ch < outputChannels; ++ch)
        wetBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    inputSmooth.setTargetValue(juce::Decibels::decibelsToGain(inputParam->load()));
    outputSmooth.setTargetValue(juce::Decibels::decibelsToGain(outputParam->load()));
    mixSmooth.setTargetValue(mixParam->load() / 100.0f);
    bypassSmooth.setTargetValue(bypassParam->load() > 0.5f ? 1.0f : 0.0f);

    if (numSamples > 0)
    {
        const float inputStart = inputSmooth.getCurrentValue();
        const float inputEnd = inputSmooth.skip(numSamples);
        for (int ch = 0; ch < outputChannels; ++ch)
            wetBuffer.applyGainRamp(ch, 0, numSamples, inputStart, inputEnd);
    }

    reverb.process(wetBuffer, (int) programParam->load(),
                   decayParam->load(), preDelayParam->load(), bassParam->load(), midParam->load(),
                   trebleParam->load(), crossoverParam->load(), depthParam->load());

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.getNextValue();
        const float mixNormalised = juce::jlimit(0.0f, 1.0f, mix);
        const float mixAngle = mixNormalised * juce::MathConstants<float>::halfPi;
        const float dryMixGain = std::cos(mixAngle);
        const float wetPresence = 1.0f + 0.22f * std::sqrt(mixNormalised);
        const float wetMixGain = std::sin(mixAngle) * wetPresence;
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
    return (int) programParam->load();
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
    ParameterMap stored;
    ParameterMap compare;
    ParameterMap slotA;
    ParameterMap slotB;
    {
        const juce::ScopedLock lock(snapshotLock);
        stored = storedSnapshot;
        compare = compareSnapshot;
        slotA = abA;
        slotB = abB;
    }

    ParameterMap customUserMap;
    {
        const juce::ScopedLock lock(snapshotLock);
        customUserMap[ParamIDs::decay] = customUserPreset.decay;
        customUserMap[ParamIDs::preDelay] = customUserPreset.preDelayMs;
        customUserMap[ParamIDs::bass] = customUserPreset.bassDb;
        customUserMap[ParamIDs::mid] = customUserPreset.midDb;
        customUserMap[ParamIDs::trebleDecay] = customUserPreset.trebleDecayDb;
        customUserMap[ParamIDs::crossover] = customUserPreset.crossoverHz;
        customUserMap[ParamIDs::depth] = customUserPreset.depthDb;
        customUserMap[ParamIDs::mix] = customUserPreset.mixPercent;
    }

    if (auto xml = createPersistedStateXml(apvts, stored, compare, slotA, slotB,
                                           abSlotB.load(std::memory_order_relaxed),
                                           comparing.load(std::memory_order_relaxed)))
    {
        xml->addChildElement(new juce::XmlElement(createParameterMapXml("CUSTOM_USER", customUserMap)));
        copyXmlToBinary(*xml, destData);
    }
}



void Borato224AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("BORATO224_STATE"))
            restorePersistedState(*xml);
        else
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            const auto restored = captureEditableParameters(apvts);
            {
                const juce::ScopedLock lock(snapshotLock);
                storedSnapshot = restored;
                compareSnapshot.clear();
                abA = restored;
                abB = restored;
            }
            abSlotB.store(false, std::memory_order_relaxed);
            comparing.store(false, std::memory_order_relaxed);
        }
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

    endCompare();

    applyingPreset.store(true, std::memory_order_relaxed);
    const auto& p = programPresets[(size_t) index];
    setParameterValue(apvts, ParamIDs::program, (float) index);

    if (index == 7)
    {
        const juce::ScopedLock lock(snapshotLock);
        setParameterValue(apvts, ParamIDs::decay, customUserPreset.decay);
        setParameterValue(apvts, ParamIDs::preDelay, customUserPreset.preDelayMs);
        setParameterValue(apvts, ParamIDs::bass, customUserPreset.bassDb);
        setParameterValue(apvts, ParamIDs::mid, customUserPreset.midDb);
        setParameterValue(apvts, ParamIDs::trebleDecay, customUserPreset.trebleDecayDb);
        setParameterValue(apvts, ParamIDs::crossover, customUserPreset.crossoverHz);
        setParameterValue(apvts, ParamIDs::depth, customUserPreset.depthDb);
        setParameterValue(apvts, ParamIDs::mix, customUserPreset.mixPercent);
        applyingPreset.store(false, std::memory_order_relaxed);
        return;
    }

    if (index == 6)
    {
        setParameterValue(apvts, ParamIDs::decay, randomRange(randomPresetRng, 1.20f, 8.80f));
        setParameterValue(apvts, ParamIDs::preDelay, randomRange(randomPresetRng, 8.0f, 115.0f));
        setParameterValue(apvts, ParamIDs::bass, randomRange(randomPresetRng, -4.5f, 4.5f));
        setParameterValue(apvts, ParamIDs::mid, randomRange(randomPresetRng, -3.5f, 3.5f));
        setParameterValue(apvts, ParamIDs::trebleDecay, randomRange(randomPresetRng, -7.5f, 2.5f));
        setParameterValue(apvts, ParamIDs::crossover, randomRange(randomPresetRng, 180.0f, 1450.0f));
        setParameterValue(apvts, ParamIDs::depth, randomRange(randomPresetRng, 1.5f, 10.0f));
        setParameterValue(apvts, ParamIDs::mix, randomRange(randomPresetRng, 18.0f, 58.0f));
    }
    else
    {
        setParameterValue(apvts, ParamIDs::decay, p.decay);
        setParameterValue(apvts, ParamIDs::preDelay, p.preDelayMs);
        setParameterValue(apvts, ParamIDs::bass, p.bassDb);
        setParameterValue(apvts, ParamIDs::mid, p.midDb);
        setParameterValue(apvts, ParamIDs::trebleDecay, p.trebleDecayDb);
        setParameterValue(apvts, ParamIDs::crossover, p.crossoverHz);
        setParameterValue(apvts, ParamIDs::depth, p.depthDb);
        setParameterValue(apvts, ParamIDs::mix, p.mixPercent);
    }
    applyingPreset.store(false, std::memory_order_relaxed);
}

void Borato224AudioProcessor::storeSnapshot()
{
    endCompare();
    const auto snapshot = captureEditableParameters(apvts);
    const juce::ScopedLock lock(snapshotLock);
    storedSnapshot = snapshot;

    if (snapshot.count(ParamIDs::decay)) customUserPreset.decay = snapshot.at(ParamIDs::decay);
    if (snapshot.count(ParamIDs::preDelay)) customUserPreset.preDelayMs = snapshot.at(ParamIDs::preDelay);
    if (snapshot.count(ParamIDs::bass)) customUserPreset.bassDb = snapshot.at(ParamIDs::bass);
    if (snapshot.count(ParamIDs::mid)) customUserPreset.midDb = snapshot.at(ParamIDs::mid);
    if (snapshot.count(ParamIDs::trebleDecay)) customUserPreset.trebleDecayDb = snapshot.at(ParamIDs::trebleDecay);
    if (snapshot.count(ParamIDs::crossover)) customUserPreset.crossoverHz = snapshot.at(ParamIDs::crossover);
    if (snapshot.count(ParamIDs::depth)) customUserPreset.depthDb = snapshot.at(ParamIDs::depth);
    if (snapshot.count(ParamIDs::mix)) customUserPreset.mixPercent = snapshot.at(ParamIDs::mix);
}

void Borato224AudioProcessor::recallSnapshot()
{
    endCompare();
    ParameterMap snapshot;
    {
        const juce::ScopedLock lock(snapshotLock);
        snapshot = storedSnapshot;
    }
    applyParameterMap(apvts, snapshot);
}

void Borato224AudioProcessor::swapAB()
{
    endCompare();

    auto current = captureEditableParameters(apvts);
    ParameterMap target;
    const bool wasSlotB = abSlotB.load(std::memory_order_relaxed);
    {
        const juce::ScopedLock lock(snapshotLock);
        if (wasSlotB)
        {
            abB = current;
            target = abA;
        }
        else
        {
            abA = current;
            target = abB;
        }
    }
    abSlotB.store(! wasSlotB, std::memory_order_relaxed);
    applyParameterMap(apvts, target);
}

void Borato224AudioProcessor::beginCompare()
{
    bool expected = false;
    if (! comparing.compare_exchange_strong(expected, true, std::memory_order_relaxed))
        return;

    const auto current = captureEditableParameters(apvts);
    ParameterMap stored;
    {
        const juce::ScopedLock lock(snapshotLock);
        compareSnapshot = current;
        stored = storedSnapshot;
    }
    applyParameterMap(apvts, stored);
}

void Borato224AudioProcessor::endCompare()
{
    if (! comparing.exchange(false, std::memory_order_relaxed))
        return;

    ParameterMap snapshotToRestore;
    {
        const juce::ScopedLock lock(snapshotLock);
        snapshotToRestore = compareSnapshot;
        compareSnapshot.clear();
    }
    applyParameterMap(apvts, snapshotToRestore);
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
            const auto value = (float) child->getDoubleAttribute("value", 0.0);
            if (id.isNotEmpty() && std::isfinite(value))
                map[id] = value;
        }
    }
    return map;
}

std::unique_ptr<juce::XmlElement> Borato224AudioProcessor::createPersistedStateXml(
    juce::AudioProcessorValueTreeState& state,
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

    root->addChildElement(state.copyState().createXml().release());
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
            if (tag != "STORED_SNAPSHOT" && tag != "COMPARE_SNAPSHOT" && tag != "AB_A" && tag != "AB_B" && tag != "CUSTOM_USER")
            {
                apvts.replaceState(juce::ValueTree::fromXml(*child));
                break;
            }

            child = child->getNextElement();
        }
    }

    const auto activeSnapshot = captureEditableParameters(apvts);
    const auto restoredStored = element.getChildByName("STORED_SNAPSHOT") != nullptr
        ? parseParameterMapXml(*element.getChildByName("STORED_SNAPSHOT"))
        : activeSnapshot;
    const auto restoredCompare = element.getChildByName("COMPARE_SNAPSHOT") != nullptr
        ? parseParameterMapXml(*element.getChildByName("COMPARE_SNAPSHOT"))
        : ParameterMap {};
    const auto restoredA = element.getChildByName("AB_A") != nullptr
        ? parseParameterMapXml(*element.getChildByName("AB_A"))
        : restoredStored;
    const auto restoredB = element.getChildByName("AB_B") != nullptr
        ? parseParameterMapXml(*element.getChildByName("AB_B"))
        : restoredStored;

    {
        const juce::ScopedLock lock(snapshotLock);
        storedSnapshot = restoredStored;
        compareSnapshot.clear();
        abA = restoredA;
        abB = restoredB;
    }

    abSlotB.store(element.getBoolAttribute("abSlotB", false), std::memory_order_relaxed);

    const bool wasComparing = element.getBoolAttribute("comparing", false);
    comparing.store(false, std::memory_order_relaxed);
    if (wasComparing && ! restoredCompare.empty())
        applyParameterMap(apvts, restoredCompare);

    if (auto* userXml = element.getChildByName("CUSTOM_USER"))
    {
        auto customUserMap = parseParameterMapXml(*userXml);
        const juce::ScopedLock lock(snapshotLock);
        if (customUserMap.count(ParamIDs::decay)) customUserPreset.decay = customUserMap[ParamIDs::decay];
        if (customUserMap.count(ParamIDs::preDelay)) customUserPreset.preDelayMs = customUserMap[ParamIDs::preDelay];
        if (customUserMap.count(ParamIDs::bass)) customUserPreset.bassDb = customUserMap[ParamIDs::bass];
        if (customUserMap.count(ParamIDs::mid)) customUserPreset.midDb = customUserMap[ParamIDs::mid];
        if (customUserMap.count(ParamIDs::trebleDecay)) customUserPreset.trebleDecayDb = customUserMap[ParamIDs::trebleDecay];
        if (customUserMap.count(ParamIDs::crossover)) customUserPreset.crossoverHz = customUserMap[ParamIDs::crossover];
        if (customUserMap.count(ParamIDs::depth)) customUserPreset.depthDb = customUserMap[ParamIDs::depth];
        if (customUserMap.count(ParamIDs::mix)) customUserPreset.mixPercent = customUserMap[ParamIDs::mix];
    }
}

void Borato224AudioProcessor::setParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value)
{
    if (auto* parameter = state.getParameter(id))
    {
        if (! std::isfinite(value))
            return;

        const float normalised = juce::jlimit(0.0f, 1.0f, parameter->convertTo0to1(value));
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
