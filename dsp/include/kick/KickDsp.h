#pragma once

#include <string_view>

namespace kick
{
// Version string of the DSP library, used by tests and the plugin's About text.
std::string_view versionString() noexcept;

// Sanity hook that proves the JUCE DSP modules are linked into kick_dsp.
// Returns the latency (in samples at the base rate) of an 8x polyphase IIR
// oversampler. Replaced by the real renderer in Milestone 2.
double oversamplerLatencySamples();
} // namespace kick
