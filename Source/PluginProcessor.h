#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "ScopeBuffers.h"
#include <atomic>

namespace token
{
class TokenAudioProcessor final : public juce::AudioProcessor
{
public:
    TokenAudioProcessor();
    ~TokenAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    ScopeFifo scopeFifo;
    std::atomic<double> currentSampleRate { 44100.0 };

    // Host transport (written on the audio thread, read by the UI for tempo-synced images)
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<double> hostPpq { 0.0 };
    std::atomic<double> hostPpqTime { 0.0 };       // Time::getMillisecondCounterHiRes() * 0.001 when hostPpq was read
    std::atomic<double> hostBeatsPerBar { 4.0 };   // in quarter notes
    std::atomic<bool>   hostPlaying { false };

    // Remembered editor sizes for each layout (saved with the session).
    std::atomic<int> stripW { defaults::stripW }, stripH { defaults::stripH };
    std::atomic<int> boxW   { defaults::boxW },   boxH   { defaults::boxH };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TokenAudioProcessor)
};
}
