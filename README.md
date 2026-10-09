# 125A Metallator V1 — Generative Industrial Percussion Synth

Early development of a **new VST3 instrument**, not an update to the old audio-input Metallator effect. The former experiment was retired; its class IDs, parameters and audio effect architecture are not reused.

## Current status

**DSP/VST3 first vertical slice only. NOT release-ready.** No custom GUI and no in-plugin WAV export yet. The native CLI reference exporter uses the exact same DSP and writes 44,100 Hz / 24-bit PCM / stereo WAVs with bounded complete tails. VST3 plugin presents controls through host generic parameter editors until the custom GUI is built.

## Sound families

- **IMPACT:** large metallic strikes and multiple recoil impulses.
- **PERC:** smaller metallic hits and brighter cymbal-like noise/resonances, without hardcoded hat/ride/crash categories.
- **FRICTION:** granular scratches through continuous stick/slip-like excitation; no forced musical pitch.
- **DRONE:** long aperiodic metal-body beds, optional MIDI pitch tracking when desired.

No sampled source audio. Sounds are generated deterministically from algorithm, preset parameters and seed. MIDI note-on fires a voice; percussive voice tails run independently of note-off; drone/friction respond to note-off with a release. Twelve polyphonic voices with deterministic voice stealing. MIDI event offsets are processed at the exact sample frame, as are registered VST3 automation queues.

## Controls and operation

ENGINE, SIZE, FORCE, CHAOS, DECAY, DRONE KEY TRACK, LEVEL, GENERATE (toggle), VARIATE (toggle), Bypass. The generic host switches are toggles: **either edge** regenerates the seed. The seed is saved with the project for repeatable results. A future GUI will expose normal buttons, sound audition, favorites and a safe explicit WAV export action outside the audio callback.

## Development toolchain

Steinberg VST3 SDK **3.8.1**, pinned by `v3.8.1_build_84`; CMake 3.25+, C++20, Windows x64 build via GitHub Actions.

```
cmake -S . -B build -DMETALLATOR_BUILD_VST3=OFF -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/metallator_render impact impact.wav 307896358
```

These commands compile the SDK-independent core and reference exporter locally. VST3 build uses `-DMETALLATOR_BUILD_VST3=ON` and downloads the pinned SDK.

## Pending release gates

Sound-quality review in musical NDH/Industrial arrangements; level-matched diversity and transient measurements; VST3 Validator; 125A QA (event I/O, state restoration, lifecycle, offline/realtime, extreme automation and Editor Lifecycle once GUI exists); actual DAW tests; full WAV export integration; readable 100/150% GUI.

All 125A development follows `challanger2000/125A-Engineering/START-HERE.md`.
