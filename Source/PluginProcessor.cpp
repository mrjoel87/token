#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace token
{
TokenAudioProcessor::TokenAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TOKEN", createParameterLayout())
{
}

void TokenAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate);
    setLatencySamples (0);
}

bool TokenAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void TokenAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // TOKEN never modifies the audio: the buffer passes through untouched.
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numIn > 0 && numSamples > 0)
    {
        const float* l = buffer.getReadPointer (0);
        const float* r = numIn > 1 ? buffer.getReadPointer (1) : l;
        scopeFifo.push (l, r, numSamples);   // lock-free, allocation-free
    }
}

juce::AudioProcessorEditor* TokenAudioProcessor::createEditor()
{
    return new TokenEditor (*this);
}

void TokenAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stripW", stripW.load(), nullptr);
    state.setProperty ("stripH", stripH.load(), nullptr);
    state.setProperty ("boxW",   boxW.load(),   nullptr);
    state.setProperty ("boxH",   boxH.load(),   nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TokenAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            stripW = (int) tree.getProperty ("stripW", defaults::stripW);
            stripH = (int) tree.getProperty ("stripH", defaults::stripH);
            boxW   = (int) tree.getProperty ("boxW",   defaults::boxW);
            boxH   = (int) tree.getProperty ("boxH",   defaults::boxH);
            apvts.replaceState (tree);
        }
    }
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new token::TokenAudioProcessor();
}
