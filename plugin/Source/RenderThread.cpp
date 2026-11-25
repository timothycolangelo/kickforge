#include "RenderThread.h"

#include "Parameters.h"
#include "kick/Renderer.h"

namespace kf
{
RenderThread::RenderThread (juce::AudioProcessorValueTreeState& s, KickBufferStore& b)
    : juce::Thread ("KICKFORGE render"), state (s), store (b)
{
    for (auto& id : allParameterIds())
        state.addParameterListener (id, this);
}

RenderThread::~RenderThread()
{
    for (auto& id : allParameterIds())
        state.removeParameterListener (id, this);
    stop();
}

void RenderThread::start()
{
    if (! isThreadRunning())
        startThread (juce::Thread::Priority::normal);
}

void RenderThread::stop()
{
    stopThread (2000);
}

void RenderThread::markDirty() noexcept
{
    lastChangeMs.store (juce::Time::getMillisecondCounterHiRes());
    dirtyGeneration.fetch_add (1);
}

void RenderThread::setSampleRateAndRenderNow (double newRate)
{
    sampleRate.store (newRate);
    renderAndPublish();
}

void RenderThread::run()
{
    while (! threadShouldExit())
    {
        wait (5);

        if (dirtyGeneration.load() == renderedGeneration.load())
            continue;
        if (juce::Time::getMillisecondCounterHiRes() - lastChangeMs.load() < debounceMs)
            continue;

        renderAndPublish();
    }
}

void RenderThread::renderAndPublish()
{
    const std::scoped_lock lock (renderMutex);

    const auto sr = sampleRate.load();
    if (sr <= 0.0)
        return;

    // Take the generation before reading params, so a change that lands
    // mid-render triggers another render afterwards.
    const auto generation = dirtyGeneration.load();
    const auto params = readKickParams (state);
    auto rendered = kick::renderKick (params, sr);

    auto buffer = std::make_unique<KickBuffer>();
    buffer->samples = std::move (rendered.samples);
    buffer->sampleRate = sr;
    buffer->hash = rendered.paramsHash;
    buffer->crossfadeSample = rendered.crossfadeSample;
    store.publish (std::move (buffer));

    renderedGeneration.store (generation);
}
} // namespace kf
