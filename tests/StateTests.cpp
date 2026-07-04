#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr float epsilon = 0.001f;

void setPlainValue(Borato224AudioProcessor& processor, const juce::String& id, float value)
{
    if (auto* parameter = processor.apvts.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

float getPlainValue(const Borato224AudioProcessor& processor, const juce::String& id)
{
    if (auto* value = processor.apvts.getRawParameterValue(id))
        return value->load();
    return std::numeric_limits<float>::quiet_NaN();
}

bool expectNear(const char* label, float actual, float expected)
{
    if (std::isfinite(actual) && std::abs(actual - expected) <= epsilon)
        return true;

    std::cerr << "FAIL: " << label << " expected " << expected << ", got " << actual << std::endl;
    return false;
}

bool testStoreRecall()
{
    Borato224AudioProcessor processor;
    setPlainValue(processor, ParamIDs::decay, 3.25f);
    processor.storeSnapshot();
    setPlainValue(processor, ParamIDs::decay, 7.50f);
    processor.recallSnapshot();
    return expectNear("STORE/RECALL", getPlainValue(processor, ParamIDs::decay), 3.25f);
}

bool testABRoundTrip()
{
    Borato224AudioProcessor processor;
    setPlainValue(processor, ParamIDs::decay, 1.50f);
    processor.swapAB();
    setPlainValue(processor, ParamIDs::decay, 4.50f);
    processor.swapAB();

    bool ok = expectNear("A/B slot A", getPlainValue(processor, ParamIDs::decay), 1.50f);
    processor.swapAB();
    ok &= expectNear("A/B slot B", getPlainValue(processor, ParamIDs::decay), 4.50f);
    return ok;
}

bool testCompare()
{
    Borato224AudioProcessor processor;
    setPlainValue(processor, ParamIDs::decay, 2.00f);
    processor.storeSnapshot();
    setPlainValue(processor, ParamIDs::decay, 6.00f);

    processor.beginCompare();
    bool ok = processor.isComparing();
    ok &= expectNear("COMPARE stored", getPlainValue(processor, ParamIDs::decay), 2.00f);

    processor.endCompare();
    ok &= ! processor.isComparing();
    ok &= expectNear("COMPARE restore", getPlainValue(processor, ParamIDs::decay), 6.00f);
    return ok;
}

bool testStateRoundTrip()
{
    Borato224AudioProcessor source;
    setPlainValue(source, ParamIDs::decay, 3.00f);
    source.storeSnapshot();
    setPlainValue(source, ParamIDs::decay, 5.00f);
    source.swapAB();
    setPlainValue(source, ParamIDs::decay, 8.00f);
    source.beginCompare();

    juce::MemoryBlock state;
    source.getStateInformation(state);

    Borato224AudioProcessor restored;
    restored.setStateInformation(state.getData(), (int) state.getSize());

    bool ok = ! restored.isComparing();
    ok &= restored.getABSlot();
    ok &= expectNear("state active B", getPlainValue(restored, ParamIDs::decay), 8.00f);

    restored.swapAB();
    ok &= expectNear("state restored A", getPlainValue(restored, ParamIDs::decay), 5.00f);
    restored.recallSnapshot();
    ok &= expectNear("state restored STORE", getPlainValue(restored, ParamIDs::decay), 3.00f);
    return ok;
}

bool testLegacyStateInitialisesSnapshots()
{
    Borato224AudioProcessor source;
    setPlainValue(source, ParamIDs::decay, 4.00f);

    juce::MemoryBlock legacyState;
    if (auto xml = source.apvts.copyState().createXml())
        juce::AudioProcessor::copyXmlToBinary(*xml, legacyState);

    Borato224AudioProcessor restored;
    restored.setStateInformation(legacyState.getData(), (int) legacyState.getSize());
    setPlainValue(restored, ParamIDs::decay, 9.00f);
    restored.recallSnapshot();

    return expectNear("legacy state STORE baseline", getPlainValue(restored, ParamIDs::decay), 4.00f);
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const bool ok = testStoreRecall()
                 && testABRoundTrip()
                 && testCompare()
                 && testStateRoundTrip()
                 && testLegacyStateInitialisesSnapshots();

    if (! ok)
        return 1;

    std::cout << "PASS: STORE/RECALL, A/B, COMPARE and state round-trip" << std::endl;
    return 0;
}
