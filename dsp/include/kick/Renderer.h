#pragma once

#include "kick/Params.h"

#include <cstdint>
#include <vector>

namespace kick
{
// Which layers to render. Tests use the single-layer modes to measure the
// tail or punch on their own.
enum class Layers
{
    both,
    punchOnly,
    tailOnly
};

struct RenderOptions
{
    Layers layers = Layers::both;
    bool applyDrive = true;
};

struct RenderedKick
{
    std::vector<float> samples; // mono
    double sampleRate = 0.0;
    std::uint64_t paramsHash = 0;
    int crossfadeSample = 0;     // where the tail starts fading in
};

// Renders one kick offline. Deterministic: the same params and sample rate
// always produce the same samples. Allocates; never call on the audio thread.
RenderedKick renderKick (const KickParams&, double sampleRate, RenderOptions = {});
} // namespace kick
