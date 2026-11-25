// kick_render: renders the default kick (or a few variations) to 24-bit mono
// WAV files without a DAW. Handy for listening tests and spectrum checks.
//
//   kick_render <out_dir> [sample_rate]

#include "kick/Renderer.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
void writeWav24 (const std::filesystem::path& path, const std::vector<float>& x, int sr)
{
    std::ofstream f (path, std::ios::binary);
    auto u32 = [&f] (std::uint32_t v) { f.write (reinterpret_cast<const char*> (&v), 4); };
    auto u16 = [&f] (std::uint16_t v) { f.write (reinterpret_cast<const char*> (&v), 2); };

    const auto dataBytes = static_cast<std::uint32_t> (x.size() * 3);
    f.write ("RIFF", 4); u32 (36 + dataBytes); f.write ("WAVE", 4);
    f.write ("fmt ", 4); u32 (16); u16 (1); u16 (1); u32 ((std::uint32_t) sr); u32 ((std::uint32_t) sr * 3); u16 (3); u16 (24);
    f.write ("data", 4); u32 (dataBytes);
    for (auto s : x)
    {
        const auto v = static_cast<std::int32_t> (std::lround (std::clamp (s, -1.0f, 1.0f) * 8388607.0f));
        const char b[3] = { (char) (v & 0xff), (char) ((v >> 8) & 0xff), (char) ((v >> 16) & 0xff) };
        f.write (b, 3);
    }
}
} // namespace

int main (int argc, char** argv)
{
    const std::filesystem::path out = argc > 1 ? argv[1] : ".";
    const auto sr = argc > 2 ? std::stoi (argv[2]) : 48000;
    std::filesystem::create_directories (out);

    struct Variant { const char* name; kick::KickParams p; };
    kick::KickParams flat, tuned, longTail;
    flat.tailStartSemis = 0.0f; flat.tailLengthMs = 300.0f; flat.tailShape = 0.7f; flat.stage1Drive = 0.9f;
    tuned.tailStartSemis = 7.0f; tuned.tailLengthMs = 400.0f; tuned.stage1Drive = 0.7f;
    longTail.tailStartSemis = 19.0f; longTail.glideTimeMs = 180.0f; longTail.tailLengthMs = 1500.0f;
    const Variant variants[] = { { "default", {} }, { "flat", flat }, { "tuned", tuned }, { "long", longTail } };

    for (const auto& v : variants)
    {
        const auto t0 = std::chrono::steady_clock::now();
        const auto k = kick::renderKick (v.p, sr);
        const auto ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        writeWav24 (out / (std::string ("KF_") + v.name + ".wav"), k.samples, sr);
        std::printf ("%-8s %6zu samples  %.1f ms render\n", v.name, k.samples.size(), ms);
    }
    return 0;
}
