#pragma once

#include "PluginProcessor.h"

// Test editor for Milestones 2-4: an audition button above JUCE's generic
// parameter list. Replaced by the custom interface in Milestone 5.
class KickforgeEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KickforgeEditor (KickforgeProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KickforgeProcessor& processor;
    juce::TextButton audition { "AUDITION" };
    juce::GenericAudioProcessorEditor generic;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickforgeEditor)
};
