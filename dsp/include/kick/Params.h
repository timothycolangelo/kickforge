#pragma once

#include <cstdint>

namespace kick
{
enum class ClickType : int
{
    noise = 0,
    chirp,
    impulse909
};

// Every value is in real units (Hz, ms, semitones, 0..1 amounts), never
// normalised plugin values. The plugin converts its parameters into this.
struct KickParams
{
    // Key ---------------------------------------------------------------
    int key = 5;               // 0 = C ... 11 = B
    int octave = 0;            // 0..2; octave 0 puts the key between C1 and B1 (32.7-61.7 Hz)
    float fineCents = 0.0f;    // -50..50

    // Punch -------------------------------------------------------------
    ClickType clickType = ClickType::impulse909;
    float clickLevel = 0.5f;   // 0..1
    float clickDecayMs = 3.0f; // 0.5..15
    float bodyStartHz = 900.0f;  // 200..2000
    float bodyEndHz = 70.0f;     // 40..150
    float sweepTimeMs = 25.0f;   // 5..80
    float sweepCurve = 0.6f;     // 0 = linear .. 1 = steep exponential
    float bodyDecayMs = 90.0f;   // 20..250
    float punchDrive = 0.3f;     // 0..1
    float punchLevelDb = 0.0f;   // -24..6

    // Tail --------------------------------------------------------------
    float tailShape = 0.35f;     // 0 sine, 1/3 triangle, 2/3 rounded square, 1 saw
    float tailStartSemis = 12.0f; // 0..48 above the key
    float glideTimeMs = 120.0f;  // 10..400
    float tailLengthMs = 550.0f; // 100..1500
    float releaseCurve = 0.5f;   // 0 = holds then drops late .. 1 = linear fade
    float tailLevelDb = 0.0f;    // -24..6

    // Layer join --------------------------------------------------------
    float crossfadePointMs = 20.0f;  // 0..80
    float crossfadeLengthMs = 10.0f; // 1..40

    // Drive (engine v1: one soft-clip stage) ------------------------------
    float stage1Drive = 0.5f;    // 0..1

    // Output ------------------------------------------------------------
    float outputGainDb = 0.0f;   // -24..12
    float fadeOutMs = 10.0f;     // 0..200
    bool normalize = true;
};

// Frequency of the tail's key (where the glide lands).
double keyFrequencyHz (const KickParams&) noexcept;

// Stable 64-bit hash of every field. Seeds the noise sources and names files.
std::uint64_t hashParams (const KickParams&) noexcept;

// The normalize target, -0.3 dBFS.
constexpr float normalizeTargetDb = -0.3f;
} // namespace kick
