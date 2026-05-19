#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"
#include "SimpleReverb.h"

class Borato224AudioProcessor final : public juce::AudioProcessor
{
public:
    Borato224AudioProcessor();
    ~Borato224AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Borato 224"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return (int) programPresets.size(); }
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

    void applyProgramPreset(int index);
    void storeSnapshot();
    void recallSnapshot();
    void swapAB();
    void beginCompare();
    void endCompare();
    bool getABSlot() const { return abSlotB; }

private:
    using ParameterMap = std::map<juce::String, float>;

    static void setParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value);
    static ParameterMap captureEditableParameters(const juce::AudioProcessorValueTreeState& state);
    static void applyParameterMap(juce::AudioProcessorValueTreeState& state, const ParameterMap& map);
    static juce::XmlElement createParameterMapXml(const juce::String& tagName, const ParameterMap& map);
    static ParameterMap parseParameterMapXml(const juce::XmlElement& element);
    static std::unique_ptr<juce::XmlElement> createPersistedStateXml(const juce::AudioProcessorValueTreeState& state,
                                                                    const ParameterMap& storedSnapshot,
                                                                    const ParameterMap& compareSnapshot,
                                                                    const ParameterMap& abA,
                                                                    const ParameterMap& abB,
                                                                    bool abSlotB,
                                                                    bool comparing);
    void restorePersistedState(const juce::XmlElement& element);
    static float randomRange(juce::Random& rng, float min, float max);

    std::atomic<float>* decayParam { nullptr };
    std::atomic<float>* preDelayParam { nullptr };
    std::atomic<float>* bassParam { nullptr };
    std::atomic<float>* midParam { nullptr };
    std::atomic<float>* trebleParam { nullptr };
    std::atomic<float>* crossoverParam { nullptr };
    std::atomic<float>* depthParam { nullptr };
    std::atomic<float>* mixParam { nullptr };
    std::atomic<float>* inputParam { nullptr };
    std::atomic<float>* outputParam { nullptr };
    std::atomic<float>* bypassParam { nullptr };

    SimpleReverb reverb;
    juce::AudioBuffer<float> wetBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmooth;

    ParameterMap storedSnapshot;
    ParameterMap compareSnapshot;
    ParameterMap abA;
    ParameterMap abB;
    juce::Random randomPresetRng { 0x2242026 };
    bool abSlotB { false };
    bool comparing { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Borato224AudioProcessor)
};
