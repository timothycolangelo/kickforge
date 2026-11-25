#include "Parameters.h"

namespace kf
{
namespace
{
using Apvts = juce::AudioProcessorValueTreeState;
using Range = juce::NormalisableRange<float>;

// Range whose midpoint sits at `centre`, which suits frequencies and times.
Range skewed (float lo, float hi, float centre, float step = 0.0f)
{
    Range r (lo, hi, step);
    r.setSkewForCentre (centre);
    return r;
}

struct FloatDef
{
    const char* id;
    const char* name;
    Range range;
    float def;
    const char* unit;
};

std::vector<FloatDef> floatDefs()
{
    return {
        { "fine", "Fine", Range (-50.0f, 50.0f, 0.1f), 0.0f, "ct" },
        { "clickLevel", "Click level", Range (0.0f, 100.0f, 0.1f), 50.0f, "%" },
        { "clickDecay", "Click decay", skewed (0.5f, 15.0f, 3.0f, 0.01f), 3.0f, "ms" },
        { "bodyStart", "Body start", skewed (200.0f, 2000.0f, 700.0f, 1.0f), 900.0f, "Hz" },
        { "bodyEnd", "Body end", skewed (40.0f, 150.0f, 80.0f, 0.1f), 70.0f, "Hz" },
        { "sweepTime", "Sweep time", skewed (5.0f, 80.0f, 25.0f, 0.1f), 25.0f, "ms" },
        { "sweepCurve", "Sweep curve", Range (0.0f, 100.0f, 0.1f), 60.0f, "%" },
        { "bodyDecay", "Body decay", skewed (20.0f, 250.0f, 90.0f, 0.1f), 90.0f, "ms" },
        { "punchDrive", "Punch drive", Range (0.0f, 100.0f, 0.1f), 30.0f, "%" },
        { "punchLevel", "Punch level", Range (-24.0f, 6.0f, 0.1f), 0.0f, "dB" },
        { "tailShape", "Shape", Range (0.0f, 100.0f, 0.1f), 35.0f, "%" },
        { "tailStart", "Tail start", Range (0.0f, 48.0f, 0.1f), 12.0f, "st" },
        { "glideTime", "Glide time", skewed (10.0f, 400.0f, 100.0f, 0.1f), 120.0f, "ms" },
        { "tailLength", "Tail length", skewed (100.0f, 1500.0f, 500.0f, 1.0f), 550.0f, "ms" },
        { "releaseCurve", "Release curve", Range (0.0f, 100.0f, 0.1f), 50.0f, "%" },
        { "tailLevel", "Tail level", Range (-24.0f, 6.0f, 0.1f), 0.0f, "dB" },
        { "xfadePoint", "Crossfade point", Range (0.0f, 80.0f, 0.1f), 20.0f, "ms" },
        { "xfadeLength", "Crossfade length", Range (1.0f, 40.0f, 0.1f), 10.0f, "ms" },
        { "stage1Drive", "Stage 1 drive", Range (0.0f, 100.0f, 0.1f), 50.0f, "%" },
        { "outGain", "Gain", Range (-24.0f, 12.0f, 0.1f), 0.0f, "dB" },
        { "fadeOut", "Fade out", Range (0.0f, 200.0f, 0.1f), 10.0f, "ms" },
    };
}

float get (const Apvts& s, const char* id)
{
    return s.getRawParameterValue (id)->load();
}
} // namespace

Apvts::ParameterLayout createParameterLayout()
{
    Apvts::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "key", 1 }, "Key",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 5));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "octave", 1 }, "Octave", 0, 2, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "clickType", 1 }, "Click type",
        juce::StringArray { "Noise", "Chirp", "909" }, 2));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "normalize", 1 }, "Normalize", true));

    for (auto& d : floatDefs())
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { d.id, 1 }, d.name, d.range, d.def,
            juce::AudioParameterFloatAttributes().withLabel (d.unit)));

    return layout;
}

const juce::StringArray& allParameterIds()
{
    static const juce::StringArray ids = []
    {
        juce::StringArray a { "key", "octave", "clickType", "normalize" };
        for (auto& d : floatDefs())
            a.add (d.id);
        return a;
    }();
    return ids;
}

kick::KickParams readKickParams (const Apvts& s)
{
    kick::KickParams p;
    p.key = juce::roundToInt (get (s, "key"));
    p.octave = juce::roundToInt (get (s, "octave"));
    p.fineCents = get (s, "fine");
    p.clickType = static_cast<kick::ClickType> (juce::roundToInt (get (s, "clickType")));
    p.clickLevel = get (s, "clickLevel") * 0.01f;
    p.clickDecayMs = get (s, "clickDecay");
    p.bodyStartHz = get (s, "bodyStart");
    p.bodyEndHz = get (s, "bodyEnd");
    p.sweepTimeMs = get (s, "sweepTime");
    p.sweepCurve = get (s, "sweepCurve") * 0.01f;
    p.bodyDecayMs = get (s, "bodyDecay");
    p.punchDrive = get (s, "punchDrive") * 0.01f;
    p.punchLevelDb = get (s, "punchLevel");
    p.tailShape = get (s, "tailShape") * 0.01f;
    p.tailStartSemis = get (s, "tailStart");
    p.glideTimeMs = get (s, "glideTime");
    p.tailLengthMs = get (s, "tailLength");
    p.releaseCurve = get (s, "releaseCurve") * 0.01f;
    p.tailLevelDb = get (s, "tailLevel");
    p.crossfadePointMs = get (s, "xfadePoint");
    p.crossfadeLengthMs = get (s, "xfadeLength");
    p.stage1Drive = get (s, "stage1Drive") * 0.01f;
    p.outputGainDb = get (s, "outGain");
    p.fadeOutMs = get (s, "fadeOut");
    p.normalize = get (s, "normalize") >= 0.5f;
    return p;
}
} // namespace kf
