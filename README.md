# KICKFORGE

A hardstyle kick generator plugin (VST3 and Standalone, plus AU on macOS). It renders the kick offline into a buffer and plays the buffer back on MIDI note-on.

Status: Milestone 1 (skeleton). The plugin builds and loads but makes no sound yet.

## Build

Requirements: CMake 3.25+, Git, and a C++20 compiler (Visual Studio 2022 or 2019 on Windows, Xcode 15+ on macOS). JUCE 8 and Catch2 are downloaded by CPM on the first configure.

```
cmake --preset win-vs2022          # or win-vs2019 / mac-xcode
cmake --build --preset win-vs2022-release
ctest --preset win-vs2022-release
```

The plugin ends up in `build/<preset>/plugin/Kickforge_artefacts/Release/VST3/KICKFORGE.vst3`.

## Layout

- `dsp/` — `kick_dsp`, the sound engine as a static library with no GUI dependency
- `plugin/` — the JUCE plugin (processor, render thread, UI)
- `resources/` — `eras.json`, factory presets, the embedded font
- `tests/` — Catch2 unit tests for `kick_dsp`
- `tools/` — pluginval and benchmark scripts
- `docs/` — `SOUND_DESIGN.md`

## Licence note

KICKFORGE is built on JUCE, which is licensed under AGPLv3 unless you hold a commercial JUCE licence. Personal use is fine; distributing a closed-source build requires a commercial JUCE licence.
