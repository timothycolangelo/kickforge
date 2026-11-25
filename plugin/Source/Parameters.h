#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "kick/Params.h"

namespace kf
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Reads the current parameter values (thread-safe: atomic reads only).
kick::KickParams readKickParams (const juce::AudioProcessorValueTreeState&);

// Every parameter ID, so listeners can be attached to all of them.
const juce::StringArray& allParameterIds();
} // namespace kf
