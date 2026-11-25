#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "KickBufferStore.h"

#include <atomic>

namespace kf
{
// Re-renders the kick in the background whenever a parameter changes,
// debounced so a fast slider drag renders once it settles (~30 ms).
class RenderThread final : private juce::Thread,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    RenderThread (juce::AudioProcessorValueTreeState&, KickBufferStore&);
    ~RenderThread() override;

    void start();
    void stop();

    // Changes the render rate and renders immediately on the calling thread,
    // so a buffer is ready before audio starts. Call from prepareToPlay.
    void setSampleRateAndRenderNow (double sampleRate);

    void markDirty() noexcept;

    static constexpr double debounceMs = 30.0;

private:
    void run() override;
    void parameterChanged (const juce::String&, float) override { markDirty(); }
    void renderAndPublish();

    juce::AudioProcessorValueTreeState& state;
    KickBufferStore& store;

    std::atomic<double> sampleRate { 0.0 };
    std::atomic<std::uint32_t> dirtyGeneration { 0 };
    std::atomic<double> lastChangeMs { 0.0 };
    std::atomic<std::uint32_t> renderedGeneration { 0 };
    std::mutex renderMutex; // serialises renders between this thread and prepareToPlay
};
} // namespace kf
