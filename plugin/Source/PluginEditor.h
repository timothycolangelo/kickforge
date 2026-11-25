#pragma once

#include "PluginProcessor.h"

// Milestone 1/2 test editor: JUCE's generic parameter list.
// Replaced by the custom interface in Milestone 5.
class KickforgeEditor final : public juce::GenericAudioProcessorEditor
{
public:
    explicit KickforgeEditor (KickforgeProcessor&);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickforgeEditor)
};
