#include "kick/Renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace kick
{
namespace
{
constexpr double twoPi = 2.0 * std::numbers::pi;

double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }

// xorshift64*: tiny, fast and fully deterministic across platforms.
struct Rng
{
    std::uint64_t state;

    explicit Rng (std::uint64_t seed) noexcept : state (seed != 0 ? seed : 0x9E3779B97F4A7C15ull) {}

    double nextBipolar() noexcept
    {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        const auto v = state * 0x2545F4914F6CDD1Dull;
        return static_cast<double> (v >> 11) * (2.0 / 9007199254740992.0) - 1.0;
    }
};

double polyBlep (double t, double dt) noexcept
{
    if (t < dt)
    {
        const auto x = t / dt;
        return x + x - x * x - 1.0;
    }
    if (t > 1.0 - dt)
    {
        const auto x = (t - 1.0) / dt;
        return x * x + x + x + 1.0;
    }
    return 0.0;
}

// The four tail waveforms. All start at 0 and rise at phase 0, like a sine,
// so morphing between them never shifts the waveform in time.
double waveSine (double p) noexcept { return std::sin (twoPi * p); }

double waveTriangle (double p) noexcept
{
    if (p < 0.25) return 4.0 * p;
    if (p < 0.75) return 2.0 - 4.0 * p;
    return 4.0 * p - 4.0;
}

double waveRoundSquare (double p) noexcept
{
    static const double norm = 1.0 / std::tanh (4.0);
    return std::tanh (4.0 * std::sin (twoPi * p)) * norm;
}

double waveSaw (double p, double dt) noexcept
{
    // Rises from 0 at p = 0, jumps from +1 to -1 at p = 0.5.
    auto t = p + 0.5;
    t -= std::floor (t);
    return 2.0 * t - 1.0 - polyBlep (t, dt);
}

double morphWave (double shape, double p, double dt) noexcept
{
    const auto s = std::clamp (shape, 0.0, 1.0) * 3.0;
    const auto seg = std::min (static_cast<int> (s), 2);
    const auto f = s - seg;

    auto wave = [p, dt] (int i)
    {
        switch (i)
        {
            case 0: return waveSine (p);
            case 1: return waveTriangle (p);
            case 2: return waveRoundSquare (p);
            default: return waveSaw (p, dt);
        }
    };

    if (f <= 0.0) return wave (seg);
    return wave (seg) * (1.0 - f) + wave (seg + 1) * f;
}

// Time constant so that exp(-t / tau) reaches -40 dB at decayMs.
double decayTau (double decayMs) noexcept { return (decayMs * 0.001) / std::log (100.0); }

// ---------------------------------------------------------------------------
void renderPunch (const KickParams& p, double sr, std::vector<double>& out, Rng& rng)
{
    const auto n = static_cast<int> (out.size());

    // Click -------------------------------------------------------------
    const auto clickTau = decayTau (p.clickDecayMs);
    const auto clickLen = std::min (n, static_cast<int> (sr * p.clickDecayMs * 0.001 * 1.5) + 1);
    const auto clickLevel = static_cast<double> (std::clamp (p.clickLevel, 0.0f, 1.0f));

    double hpState = 0.0, hpPrevIn = 0.0;
    const auto hpR = std::exp (-twoPi * 1500.0 / sr);
    double lpState = 0.0;
    const auto lpA = 1.0 - std::exp (-twoPi * 3000.0 / sr);
    double chirpPhase = 0.0;
    const auto pulseLen = std::max (1.0, sr * 0.0004);

    for (int i = 0; i < clickLen; ++i)
    {
        const auto t = i / sr;
        const auto env = std::exp (-t / clickTau);
        double v = 0.0;

        switch (p.clickType)
        {
            case ClickType::noise:
            {
                const auto x = rng.nextBipolar();
                hpState = hpR * (hpState + x - hpPrevIn); // one-pole high-pass
                hpPrevIn = x;
                v = hpState * env;
                break;
            }
            case ClickType::chirp:
            {
                const auto x = std::min (1.0, t / (p.clickDecayMs * 0.001));
                const auto f = 6000.0 * std::pow (300.0 / 6000.0, x);
                v = std::sin (twoPi * chirpPhase) * env;
                chirpPhase += f / sr;
                chirpPhase -= std::floor (chirpPhase);
                break;
            }
            case ClickType::impulse909:
            {
                // Short positive half-sine pulse plus a little low-passed noise.
                const auto pulse = (i < pulseLen) ? std::sin (std::numbers::pi * i / pulseLen) : 0.0;
                lpState += lpA * (rng.nextBipolar() - lpState);
                v = pulse + 0.35 * lpState * env;
                break;
            }
        }

        out[(size_t) i] += clickLevel * v;
    }

    // Body: a sine with a fast exponential pitch sweep -----------------------
    const auto start = static_cast<double> (std::max (p.bodyStartHz, p.bodyEndHz));
    const auto end = static_cast<double> (p.bodyEndHz);
    const auto sweep = std::max (0.001, p.sweepTimeMs * 0.001);
    const auto curveK = 1.0 + 7.0 * std::clamp (static_cast<double> (p.sweepCurve), 0.0, 1.0);
    const auto bodyTau = decayTau (p.bodyDecayMs);
    const auto bodyLen = std::min (n, static_cast<int> (sr * p.bodyDecayMs * 0.001 * 2.0) + 1);

    double phase = 0.0;
    for (int i = 0; i < bodyLen; ++i)
    {
        const auto t = i / sr;
        const auto x = std::min (1.0, t / sweep);
        const auto f = end + (start - end) * std::pow (1.0 - x, curveK);
        out[(size_t) i] += std::sin (twoPi * phase) * std::exp (-t / bodyTau);
        phase += f / sr;
        phase -= std::floor (phase);
    }

    // Punch drive: soft clip ------------------------------------------------
    const auto drive = std::clamp (static_cast<double> (p.punchDrive), 0.0, 1.0);
    if (drive > 0.0)
    {
        const auto g = dbToGain (drive * 24.0);
        const auto norm = 1.0 / std::tanh (g);
        for (auto& s : out)
            s = std::tanh (g * s) * norm;
    }

    const auto level = dbToGain (p.punchLevelDb);
    for (auto& s : out)
        s *= level;
}

// ---------------------------------------------------------------------------
void renderTail (const KickParams& p, double sr, std::vector<double>& out, int startSample)
{
    const auto n = static_cast<int> (out.size());
    const auto keyHz = keyFrequencyHz (p);
    const auto startSemis = static_cast<double> (std::clamp (p.tailStartSemis, 0.0f, 48.0f));
    const auto glide = std::max (0.001, p.glideTimeMs * 0.001);
    const auto len = std::max (0.001, p.tailLengthMs * 0.001);
    const auto releaseExp = 8.0 - 7.0 * std::clamp (static_cast<double> (p.releaseCurve), 0.0, 1.0);
    const auto level = dbToGain (p.tailLevelDb);
    const auto shape = static_cast<double> (p.tailShape);

    double phase = 0.0;
    for (int i = startSample; i < n; ++i)
    {
        const auto t = (i - startSample) / sr;
        if (t >= len)
            break;

        // Glide: starts startSemis above the key and lands exactly on it.
        const auto x = std::min (1.0, t / glide);
        const auto semis = startSemis * (1.0 - x) * (1.0 - x);
        const auto f = keyHz * std::pow (2.0, semis / 12.0);
        const auto dt = f / sr;

        const auto amp = 1.0 - std::pow (t / len, releaseExp);
        out[(size_t) i] += level * amp * morphWave (shape, phase, dt);

        phase += dt;
        phase -= std::floor (phase);
    }
}
} // namespace

// ---------------------------------------------------------------------------
double keyFrequencyHz (const KickParams& p) noexcept
{
    const auto midi = 24 + 12 * std::clamp (p.octave, 0, 2) + std::clamp (p.key, 0, 11);
    return 440.0 * std::pow (2.0, (midi - 69 + p.fineCents / 100.0) / 12.0);
}

std::uint64_t hashParams (const KickParams& p) noexcept
{
    std::uint64_t h = 0xcbf29ce484222325ull; // FNV-1a
    auto mix = [&h] (const void* data, size_t size)
    {
        const auto* bytes = static_cast<const unsigned char*> (data);
        for (size_t i = 0; i < size; ++i)
        {
            h ^= bytes[i];
            h *= 0x100000001b3ull;
        }
    };
    auto f = [&mix] (float v) { mix (&v, sizeof v); };
    auto i = [&mix] (int v) { mix (&v, sizeof v); };

    i (p.key); i (p.octave); f (p.fineCents);
    i (static_cast<int> (p.clickType)); f (p.clickLevel); f (p.clickDecayMs);
    f (p.bodyStartHz); f (p.bodyEndHz); f (p.sweepTimeMs); f (p.sweepCurve);
    f (p.bodyDecayMs); f (p.punchDrive); f (p.punchLevelDb);
    f (p.tailShape); f (p.tailStartSemis); f (p.glideTimeMs); f (p.tailLengthMs);
    f (p.releaseCurve); f (p.tailLevelDb);
    f (p.crossfadePointMs); f (p.crossfadeLengthMs);
    f (p.stage1Drive);
    f (p.outputGainDb); f (p.fadeOutMs); i (p.normalize ? 1 : 0);
    return h;
}

RenderedKick renderKick (const KickParams& p, double sr, RenderOptions options)
{
    RenderedKick result;
    result.sampleRate = sr;
    result.paramsHash = hashParams (p);

    const auto xfStart = static_cast<int> (std::lround (std::clamp (p.crossfadePointMs, 0.0f, 80.0f) * 0.001 * sr));
    const auto xfLen = std::max (1, static_cast<int> (std::lround (std::clamp (p.crossfadeLengthMs, 1.0f, 40.0f) * 0.001 * sr)));
    const auto tailLen = static_cast<int> (std::lround (std::clamp (p.tailLengthMs, 100.0f, 1500.0f) * 0.001 * sr));
    const auto total = xfStart + tailLen;
    result.crossfadeSample = xfStart;

    std::vector<double> punch ((size_t) total, 0.0), tail ((size_t) total, 0.0);
    Rng rng (result.paramsHash);

    if (options.layers != Layers::tailOnly)
        renderPunch (p, sr, punch, rng);
    if (options.layers != Layers::punchOnly)
        renderTail (p, sr, tail, xfStart);

    // Equal-power crossfade: the punch fades out while the tail fades in.
    std::vector<double> mix ((size_t) total);
    for (int i = 0; i < total; ++i)
    {
        const auto x = std::clamp ((i - xfStart) / static_cast<double> (xfLen), 0.0, 1.0);
        const auto gPunch = options.layers == Layers::punchOnly ? 1.0 : std::cos (0.5 * std::numbers::pi * x);
        const auto gTail = options.layers == Layers::tailOnly ? 1.0 : std::sin (0.5 * std::numbers::pi * x);
        mix[(size_t) i] = punch[(size_t) i] * gPunch + tail[(size_t) i] * gTail;
    }

    // Drive chain, engine v1: one tanh soft-clip stage.
    const auto drive = std::clamp (static_cast<double> (p.stage1Drive), 0.0, 1.0);
    if (options.applyDrive && drive > 0.0)
    {
        const auto g = dbToGain (drive * 30.0);
        for (auto& s : mix)
            s = std::tanh (g * s);
    }

    // DC removal: one-pole high-pass at 10 Hz.
    {
        const auto r = std::exp (-twoPi * 10.0 / sr);
        double y = 0.0, xPrev = 0.0;
        for (auto& s : mix)
        {
            y = s - xPrev + r * y;
            xPrev = s;
            s = y;
        }
    }

    // Output gain and fade-out.
    const auto gain = dbToGain (p.outputGainDb);
    const auto fadeLen = std::min (total, static_cast<int> (std::lround (std::max (0.0f, p.fadeOutMs) * 0.001 * sr)));
    for (int i = 0; i < total; ++i)
    {
        auto g = gain;
        const auto fromEnd = total - 1 - i;
        if (fromEnd < fadeLen)
            g *= 0.5 - 0.5 * std::cos (std::numbers::pi * fromEnd / static_cast<double> (fadeLen));
        mix[(size_t) i] *= g;
    }

    // The 10 Hz high-pass can't settle within a short kick, so a small offset
    // remains. Remove what's left with a Hann-shaped correction: it is zero at
    // both ends (no step at the start or end) and sits far below 10 Hz.
    if (total > 2)
    {
        double sum = 0.0, wSum = 0.0;
        std::vector<double> w ((size_t) total);
        for (int i = 0; i < total; ++i)
        {
            w[(size_t) i] = 0.5 - 0.5 * std::cos (twoPi * i / (total - 1));
            sum += mix[(size_t) i];
            wSum += w[(size_t) i];
        }
        const auto k = sum / wSum;
        for (int i = 0; i < total; ++i)
            mix[(size_t) i] -= k * w[(size_t) i];
    }

    if (p.normalize)
    {
        double peak = 0.0;
        for (auto s : mix)
            peak = std::max (peak, std::abs (s));
        if (peak > 1.0e-9)
        {
            // A hair under the target so float rounding can never push it over.
            const auto scale = dbToGain (normalizeTargetDb) * 0.9999 / peak;
            for (auto& s : mix)
                s *= scale;
        }
    }

    result.samples.resize ((size_t) total);
    for (int i = 0; i < total; ++i)
        result.samples[(size_t) i] = static_cast<float> (mix[(size_t) i]);
    return result;
}
} // namespace kick
