# 125A Metallator V1 — Generative Industrial Percussion Synth

Early development of a **new VST3 instrument**, not an update to the old audio-input Metallator effect. The former experiment was retired; its class IDs, parameters and audio effect architecture are not reused.

## Current status

**Offline sound-design candidate with structural GENERATE and VARIATE. NOT release-ready.** No custom GUI and no in-plugin WAV export yet. The native CLI reference exporter uses the exact same DSP and writes 44,100 Hz / 24-bit PCM / stereo WAVs with bounded complete tails. VST3 plugin presents controls through host generic parameter editors until the custom GUI is built.

## Sound families

- **IMPACT:** one heavy steel strike with 84 independently decaying, inharmonic modes;
  a broadband contact bite and a decaying, unpitched pressure wave. **No synthetic
  secondary collisions, tempo echoes or rebound sequences.** Seed changes body
  geometry and modal spectrum. FORCE 0% is silent. This new design is still under
  listening review, and is not a physically calibrated steel simulation.
- **PERC:** four different hit/chain-link/sheet/ratchet constructions, not a pitch-shifted standard cymbal.
- **FRICTION:** rasp, tearing contact, slow metal drag and abrasive stressed contact; no forced musical pitch.
- **DRONE:** massive body, stressed plate, slow pressure and irregular motor structures; optional pitched modes only when enabled.

No sampled source audio. Each engine contains four structurally different archetypes. GENERATE changes archetype; VARIATE changes microgeometry within that archetype. See [technical evidence](docs/GENERATIVE-ARCHITECTURE.md). Sounds are generated deterministically from algorithm, preset parameters, archetype and seed. MIDI note-on fires a voice; percussive voice tails run independently of note-off; drone/friction respond to note-off with a release. Twelve polyphonic voices with deterministic voice stealing. MIDI event offsets are processed at the exact sample frame, as are registered VST3 automation queues.

## Controls and operation

ENGINE, SIZE, FORCE, CHAOS, DECAY, DRONE KEY TRACK, LEVEL, GENERATE (toggle), VARIATE (toggle), Bypass. The generic host switches are toggles: **either edge** regenerates the seed.
The change is heard on the **next MIDI Note On**; holding a note does not restart
an existing metallic tail. Trigger another note after using GENERATE / VARIATE. The seed and archetype are saved with the project for repeatable results; state V2 can load the original state V1 format. A future GUI will expose normal buttons, sound audition, favorites and a safe explicit WAV export action outside the audio callback.

## Development toolchain

Steinberg VST3 SDK **3.8.1**, pinned by `v3.8.1_build_84`; CMake 3.25+, C++20, Windows x64 build via GitHub Actions.

```
cmake -S . -B build -DMETALLATOR_BUILD_VST3=OFF -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/metallator_render impact impact.wav 307896358 0
./build/metallator_render friction friction-rip.wav 307896358 1
```

These commands compile the SDK-independent core and reference exporter locally. VST3 build uses `-DMETALLATOR_BUILD_VST3=ON` and downloads the pinned SDK.

## Impact rebuild / single-strike QA (development only)

The replacement IMPACT synthesizer was derived from the independently scripted
`01_Stahltraeger_Kollision.wav` experiment, keeping its distinct low/mid/high
modal bands and contact/pressure excitation. The separate echo-like rebounds,
loose-debris impacts, artificial room reflections and offline nonlinear mastering
were deliberately **not** ported. Resonant modal physics, exponential T60 and
bounded oscillator recursion are documented/derived; modal frequency spread,
mechanical sound design and output voicing are **EMPIRICALLY TUNED**.

The core test suite now uses checks that execute in **Release** builds (unlike
ordinary C++ `assert` under `NDEBUG`). A separate IMPACT contract test verifies
sample rates, long-lasting body, force response, atonal MIDI triggers, 0% FORCE
silence, output bounds and deterministic seed changes. The full host and sonic
release gates remain OPEN.

## Pending release gates

Structural C++ offline QA passed locally, but real audio listening is OPEN. Sound-quality review in musical NDH/Industrial arrangements; level-matched diversity and transient measurements; VST3 Validator; 125A QA (event I/O, state restoration, lifecycle, offline/realtime, extreme automation and Editor Lifecycle once GUI exists); actual DAW tests; full WAV export integration; readable 100/150% GUI.

All 125A development follows `challanger2000/125A-Engineering/START-HERE.md`.
