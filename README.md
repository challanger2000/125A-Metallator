# 125A Metallator V1 — Metal Percussion Instrument (development)

**Target:** synthetic metallic drum replacements for Industrial / NDH / Metal.
This is a focused percussion instrument, **not** a generic metal ambience or drone synthesizer.

## Current active engines

- **IMPACT:** big atonal metal impacts for heavy kick/snare/tom-like accents and massive hits. One initial strike, no tempo echoes or additional staged impact triggers. Four constructions: the earlier approved 84-mode steel hit, steel block, steel cage, suspended plate.
- **PERC:** atonal, short/medium metal percussion: ticks, rattling chain links, sheet clicks and industrial ratchets. Four constructions.

**FRICTION and DRONE have been removed from both the real-time audio engine and the instrument editor.** No sustaining drone, scraping/pitch oscillator or pitch-tracking mode is available. All MIDI notes trigger sound events without musical pitch. NoteOff does not kill the percussive tail; allNotesOff is a panic operation. Multiple MIDI notes can overlap.

GENERATE cycles a new structural archetype within IMPACT/PERC, VARIATE changes the chosen archetype's microgeometry. No promise of an unlimited sound vocabulary: currently 8 built-in archetypes, under sonic development.

## Project/state compatibility

Existing stable VST3 processor/controller class IDs and parameter IDs are retained. State schema V3 reads V1/V2/V3.
- Old engine 0 IMPACT → IMPACT.
- Old engine 1 PERC → PERC.
- Old engine 2 FRICTION → PERC (replaced; cannot faithfully reproduce old audio).
- Old engine 3 DRONE → IMPACT (replaced; cannot faithfully reproduce old audio).
- Legacy key-track parameter ID 1005 and serialized field are retained for state compatibility but are **hidden/inert**. Engine selector offers exactly two entries.
- DAW automation lanes recorded with the former *four-position* ENGINE normalized scale require manual review; the new two-position mapping intentionally changes that range in this unreleased prototype. No claim of full audio or automation backward compatibility.

## Status and next work

This is an internal DSP-focused development branch. A professional 125A GUI, preset workflow, working export, exact host lifecycle/state tests, sound design and true realtime qualification are **OPEN**. No user testing required until a significantly more finished instrument with a proper GUI exists.

Source of truth: `challanger2000/125A-Engineering/START-HERE.md`; 125A logo/knobs must come from the Branding and Knob Designer master repositories. GUI static design FIRST, then VSTGUI.

## Local verification (no GitHub Actions required)

`cmake -S . -B build -DMETALLATOR_BUILD_VST3=OFF -DBUILD_TESTING=ON` 
`cmake --build build --config Release` 
`ctest --test-dir build --output-on-failure`

No Windows/Studio One or sonically approved release claim is implied by offline tests.
