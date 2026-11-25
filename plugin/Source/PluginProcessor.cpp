#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

KickforgeProcessor::KickforgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KICKFORGE", kf::createParameterLayout()),
      renderThread (apvts, bufferStore)
{
    renderThread.start();
}

KickforgeProcessor::~KickforgeProcessor()
{
    renderThread.stop();
}

void KickforgeProcessor::prepareToPlay (double sampleRate, int)
{
    voicePos = -1;
    voiceBuffer = nullptr;
    playheadForGui.store (-1);
    renderThread.setSampleRateAndRenderNow (sampleRate);
}

bool KickforgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // The kick is mono; we offer mono or stereo (the same signal on both).
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void KickforgeProcessor::startVoice() noexcept
{
    voiceBuffer = bufferStore.acquireForPlayback();
    voicePos = voiceBuffer != nullptr ? 0 : -1;
}

void KickforgeProcessor::renderVoice (juce::AudioBuffer<float>& buffer, int start, int num) noexcept
{
    if (voicePos < 0 || voiceBuffer == nullptr || num <= 0)
        return;

    const auto& src = voiceBuffer->samples;
    const auto available = static_cast<int> (src.size()) - voicePos;
    const auto n = juce::jmin (num, available);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, start, src.data() + voicePos, n);

    voicePos += n;
    if (voicePos >= static_cast<int> (src.size()))
        voicePos = -1;
}

void KickforgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto numSamples = buffer.getNumSamples();
    int pos = 0;

    if (auditionRequested.exchange (false))
        startVoice();

    // Split the block at every note-on so retriggers are sample-accurate.
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (! msg.isNoteOn())
            continue;

        const auto at = juce::jlimit (0, numSamples, metadata.samplePosition);
        renderVoice (buffer, pos, at - pos);
        pos = at;
        startVoice();
    }

    renderVoice (buffer, pos, numSamples - pos);
    playheadForGui.store (voicePos);
}

juce::AudioProcessorEditor* KickforgeProcessor::createEditor()
{
    return new KickforgeEditor (*this);
}

void KickforgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KickforgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickforgeProcessor();
}
