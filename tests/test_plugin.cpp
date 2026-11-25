// Integration tests for the plugin's audio path: MIDI in, rendered kick out.
#include <catch2/catch_test_macros.hpp>

#include "PluginProcessor.h"

#include <cmath>

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 256;

float peakOf (const juce::AudioBuffer<float>& b, int ch)
{
    return b.getMagnitude (ch, 0, b.getNumSamples());
}
} // namespace

TEST_CASE ("note-on plays the rendered kick, sample-accurately", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    KickforgeProcessor proc;
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;

    // Silence before any note.
    proc.processBlock (buffer, midi);
    REQUIRE (peakOf (buffer, 0) == 0.0f);

    // Note-on at sample 100: nothing before it, the kick's first samples after.
    midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 100), 100);
    proc.processBlock (buffer, midi);
    REQUIRE (buffer.getMagnitude (0, 0, 100) == 0.0f);

    {
        auto lock = proc.getBufferStore().lockForReading();
        const auto* kick = proc.getBufferStore().latestUnsafe();
        REQUIRE (kick != nullptr);
        for (int i = 0; i < block - 100; ++i)
        {
            REQUIRE (buffer.getSample (0, 100 + i) == kick->samples[(size_t) i]);
            REQUIRE (buffer.getSample (1, 100 + i) == kick->samples[(size_t) i]);
        }
    }

    // Keeps playing in later blocks, then goes silent once the kick ends.
    midi.clear();
    proc.processBlock (buffer, midi);
    REQUIRE (peakOf (buffer, 0) > 0.1f);

    for (int i = 0; i < 400; ++i)
        proc.processBlock (buffer, midi);
    REQUIRE (peakOf (buffer, 0) == 0.0f);
    REQUIRE (proc.getPlayheadSample() == -1);
}

TEST_CASE ("parameter changes re-render in the background", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    KickforgeProcessor proc;
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    auto& store = proc.getBufferStore();
    const auto firstHash = [&] { auto l = store.lockForReading(); return store.latestUnsafe()->hash; }();
    const auto firstLen = [&] { auto l = store.lockForReading(); return store.latestUnsafe()->samples.size(); }();

    auto* tailLength = proc.apvts.getParameter ("tailLength");
    tailLength->setValueNotifyingHost (tailLength->convertTo0to1 (1200.0f));

    // Debounce is 30 ms; give the render thread up to 2 s.
    bool changed = false;
    for (int i = 0; i < 200 && ! changed; ++i)
    {
        juce::Thread::sleep (10);
        auto l = store.lockForReading();
        changed = store.latestUnsafe()->hash != firstHash;
    }
    REQUIRE (changed);

    auto l = store.lockForReading();
    REQUIRE (store.latestUnsafe()->samples.size() > firstLen);
}

TEST_CASE ("audition plays without MIDI", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    KickforgeProcessor proc;
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    proc.triggerAudition();
    proc.processBlock (buffer, midi);
    REQUIRE (peakOf (buffer, 0) > 0.0f);
}
