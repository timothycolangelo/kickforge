#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "KickBufferStore.h"
#include "RenderThread.h"

class KickforgeProcessor final : public juce::AudioProcessor
{
public:
    KickforgeProcessor();
    ~KickforgeProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Plays the kick from the start on the next audio block (GUI audition).
    void triggerAudition() noexcept { auditionRequested.store (true); }

    // Playback position in samples, or -1 when idle. For the GUI playhead.
    int getPlayheadSample() const noexcept { return playheadForGui.load(); }

    kf::KickBufferStore& getBufferStore() noexcept { return bufferStore; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void startVoice() noexcept;
    void renderVoice (juce::AudioBuffer<float>&, int start, int num) noexcept;

    kf::KickBufferStore bufferStore;
    kf::RenderThread renderThread;

    // Audio-thread voice state.
    const kf::KickBuffer* voiceBuffer = nullptr;
    int voicePos = -1;

    std::atomic<bool> auditionRequested { false };
    std::atomic<int> playheadForGui { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickforgeProcessor)
};
