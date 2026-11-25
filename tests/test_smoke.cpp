#include <catch2/catch_test_macros.hpp>

#include "kick/KickDsp.h"

TEST_CASE ("kick_dsp links and reports its version", "[smoke]")
{
    REQUIRE (kick::versionString() == "0.1.0");
}

TEST_CASE ("JUCE 8x oversampler is available inside kick_dsp", "[smoke]")
{
    const auto latency = kick::oversamplerLatencySamples();
    REQUIRE (latency > 0.0);
    REQUIRE (latency < 64.0);
}
