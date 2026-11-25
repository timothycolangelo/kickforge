#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "kick/KickDsp.h"

KickforgeProcessor::KickforgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KICKFORGE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout KickforgeProcessor::createLayout()
{
    // Milestone 1 placeholder. The real parameter set arrives in Milestone 2.
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gain", 1 }, "Gain",
                                                             juce::NormalisableRange<float> (-24.0f, 6.0f, 0.01f), 0.0f));
    return layout;
}

void KickforgeProcessor::prepareToPlay (double, int) {}

bool KickforgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // The kick is mono; we offer mono or stereo (duplicated) output.
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void KickforgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
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
