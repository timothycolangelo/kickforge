#include "kick/KickDsp.h"

#include <juce_dsp/juce_dsp.h>

namespace kick
{
std::string_view versionString() noexcept
{
    return "0.1.0";
}

double oversamplerLatencySamples()
{
    // 3 stages of 2x = 8x.
    juce::dsp::Oversampling<float> os (1, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    os.initProcessing (512);
    return static_cast<double> (os.getLatencyInSamples());
}
} // namespace kick
