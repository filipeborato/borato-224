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

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.getNextValue();
        const float bypass = bypassSmooth.getNextValue();
        const float outGain = outputSmooth.getNextValue();

        for (int ch = 0; ch < outputChannels; ++ch)
        {
            auto* dry = buffer.getWritePointer(ch);
            auto* wet = wetBuffer.getReadPointer(ch);
            const float effected = dry[i] * (1.0f - mix) + wet[i] * mix;
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
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void Borato224AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
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
