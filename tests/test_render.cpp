#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "kick/Renderer.h"

#include <cmath>
#include <cstring>
#include <random>

using namespace kick;

namespace
{
constexpr double sr = 48000.0;

KickParams randomParams (std::mt19937& g)
{
    auto u = [&g] (float lo, float hi) { return std::uniform_real_distribution<float> (lo, hi) (g); };
    auto ui = [&g] (int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (g); };

    KickParams p;
    p.key = ui (0, 11);
    p.octave = ui (0, 2);
    p.fineCents = u (-50, 50);
    p.clickType = static_cast<ClickType> (ui (0, 2));
    p.clickLevel = u (0, 1);
    p.clickDecayMs = u (0.5f, 15);
    p.bodyStartHz = u (200, 2000);
    p.bodyEndHz = u (40, 150);
    p.sweepTimeMs = u (5, 80);
    p.sweepCurve = u (0, 1);
    p.bodyDecayMs = u (20, 250);
    p.punchDrive = u (0, 1);
    p.punchLevelDb = u (-24, 6);
    p.tailShape = u (0, 1);
    p.tailStartSemis = u (0, 48);
    p.glideTimeMs = u (10, 400);
    p.tailLengthMs = u (100, 1500);
    p.releaseCurve = u (0, 1);
    p.tailLevelDb = u (-24, 6);
    p.crossfadePointMs = u (0, 80);
    p.crossfadeLengthMs = u (1, 40);
    p.stage1Drive = u (0, 1);
    p.outputGainDb = u (-24, 12);
    p.fadeOutMs = u (0, 200);
    p.normalize = ui (0, 1) == 1;
    return p;
}

// Frequency from rising zero crossings (linearly interpolated) in [from, to).
double measureFrequency (const std::vector<float>& x, int from, int to)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (int i = std::max (1, from); i < to; ++i)
    {
        if (x[(size_t) i - 1] < 0.0f && x[(size_t) i] >= 0.0f)
        {
            const auto a = x[(size_t) i - 1], b = x[(size_t) i];
            const auto t = (i - 1) + a / (a - b);
            if (first < 0.0) first = t;
            last = t;
            ++count;
        }
    }
    REQUIRE (count >= 3);
    return (count - 1) * sr / (last - first);
}

double cents (double f, double ref) { return 1200.0 * std::log2 (f / ref); }
} // namespace

TEST_CASE ("tail glide lands within 1 cent of the key", "[pitch]")
{
    const auto shape = GENERATE (0.0f, 0.5f, 1.0f);
    const auto key = GENERATE (0, 5, 9, 11);
    const auto octave = GENERATE (0, 1);

    KickParams p;
    p.key = key;
    p.octave = octave;
    p.tailShape = shape;
    p.tailStartSemis = 24.0f;
    p.glideTimeMs = 150.0f;
    p.tailLengthMs = 900.0f;
    p.fadeOutMs = 0.0f;
    p.normalize = false;

    const auto k = renderKick (p, sr, { Layers::tailOnly, false });

    // Last stable section: from 100 ms after the glide ends to 85 % of the tail.
    const auto from = k.crossfadeSample + static_cast<int> ((p.glideTimeMs + 100.0f) * 0.001 * sr);
    const auto to = k.crossfadeSample + static_cast<int> (p.tailLengthMs * 0.85f * 0.001 * sr);
    const auto f = measureFrequency (k.samples, from, to);

    INFO ("shape " << shape << " key " << key << " octave " << octave << " measured " << f << " Hz, target " << keyFrequencyHz (p));
    REQUIRE (std::abs (cents (f, keyFrequencyHz (p))) <= 1.0);
}

TEST_CASE ("500 random parameter sets: finite output and no DC above -60 dBFS", "[robust]")
{
    std::mt19937 g (1234);
    const auto dcLimit = std::pow (10.0, -60.0 / 20.0);

    for (int n = 0; n < 500; ++n)
    {
        const auto p = randomParams (g);
        const auto k = renderKick (p, sr);
        REQUIRE (! k.samples.empty());

        double sum = 0.0;
        bool finite = true;
        for (auto s : k.samples)
        {
            finite = finite && std::isfinite (s);
            sum += s;
        }
        const auto dc = std::abs (sum / static_cast<double> (k.samples.size()));

        INFO ("set " << n << " dc " << dc);
        REQUIRE (finite);
        REQUIRE (dc < dcLimit);
    }
}

TEST_CASE ("normalize keeps the peak at or below -0.3 dBFS", "[normalize]")
{
    std::mt19937 g (99);
    const auto limit = std::pow (10.0f, -0.3f / 20.0f);

    for (int n = 0; n < 100; ++n)
    {
        auto p = randomParams (g);
        p.normalize = true;
        const auto k = renderKick (p, sr);

        float peak = 0.0f;
        for (auto s : k.samples)
            peak = std::max (peak, std::abs (s));

        INFO ("set " << n << " peak " << peak);
        REQUIRE (peak <= limit);
        REQUIRE (peak > limit * 0.99f); // and it really was normalised
    }
}

TEST_CASE ("rendering is deterministic", "[determinism]")
{
    std::mt19937 g (7);
    for (int n = 0; n < 20; ++n)
    {
        auto p = randomParams (g);
        p.clickType = static_cast<ClickType> (n % 3); // make sure the noise clicks are covered
        const auto a = renderKick (p, sr);
        const auto b = renderKick (p, sr);
        REQUIRE (a.samples.size() == b.samples.size());
        REQUIRE (std::memcmp (a.samples.data(), b.samples.data(), a.samples.size() * sizeof (float)) == 0);
    }
}

TEST_CASE ("different parameters give a different hash", "[determinism]")
{
    KickParams a, b;
    b.clickLevel += 0.01f;
    REQUIRE (hashParams (a) != hashParams (b));
    REQUIRE (hashParams (a) == hashParams (KickParams {}));
}
