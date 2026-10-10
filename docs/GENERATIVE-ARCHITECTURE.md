> **HISTORICAL — previous four-engine sound-design research, not the current product specification.** See [FOCUS-2026-10-10.md](FOCUS-2026-10-10.md).

# Metallator V1 — Structural GENERATE / bounded VARIATE (10 Oct 2026)

Status: **offline C++ sound-design candidate**, **NOT** VST3 / DAW / musical approval.
Development base: `challanger2000/125A-Metallator`, branch `v1.0.0-synth-dev`, baseline `f529d53b5d09145040d0d32b9450be6a61fe4fa7`.

## Problem & observed evidence (MEASURED)

User auditioned four nominal engines but found approximately four usable sound identities, with GENERATE/VARIATE apparently changing mostly pitch/timbre. Initial 16-seed baseline (2-second temporal-envelope cosine median): IMPACT .966, PERC 1.000, FRICTION .995, DRONE .937. Raw seeded sample difference is not a meaningful evidence of structural variety.

## Intent & architecture

- **IMPACT** — four distinct structures: (0) accepted 84-mode steel collision, (1) low steel-block compression, (2) cage snap, (3) suspended metal plate. No synthetic echo, secondary impact or tempo sync for IMPACT.
- **PERC** — (0) isolated metal tick, (1) irregular multiple chain-link contacts, (2) heavy sheet click, (3) mechanically slowing ratchet.
- **FRICTION** — (0) coarse rasp, (1) discrete tearing contacts, (2) heavy slow drag, (3) abrasive high-frequency contact. No pure sinewave movement driver; resonances remain inharmonic, atonal by default.
- **DRONE** — (0) dense steel-body resonance, (1) stressed plate, (2) slow pneumatic pressure, (3) corroded motor. Independent continuous excitation, metal-resonator modes and irregular internal contact events.

**GENERATE** changes `Patch::archetype` (cycles 0→1→2→3→0) and fine-geometry seed. Every activation guarantees a changed structural archetype. It applies to newly triggered voices, not retroactively to sounding tails.

**VARIATE** changes the microgeometry seed within the selected archetype; a separate seeded event scheduler preserves the characteristic macro contact timing across variations.

These are currently **four curated construction families per engine (16 total)**, not an unlimited synthesis vocabulary. Distinct seeds alter geometry and noise textures within each. Future expansion must earn its place by independent measurement and musical review.

`Patch::keyTrack` applies only to DRONE if manually enabled. All engines default to atonal note triggering, with no forced MIDI key pitch. Note velocity affects gain. All noise/contact schedules are deterministic and thread-local; no samples, filesystem, locks or allocations in render.

## DSP approach (EMPIRICALLY TUNED)

Time-varying contact excitation feeds inharmonic two-pole modal resonators. Parameters are established at note-on (damping, modal frequencies/weights, filter poles). The per-sample path uses precomputed recurrences and bounded scheduler steps. Different architectures use *different modal band occupancy*, event pulse scheduling, attack/release, excitation band mixing and source response; they do not merely scale one master fundamental. Numbers are empirical design constants, not measured material properties, and should not be described as a physically faithful reconstruction of a specific machine.

The original accepted impact mode `impact_body.h` remains selected by IMPACT archetype 0. Subsequent impact archetypes use `mechanical_body.h` with one initial contact and no timed re-strikes.

## State migration and host behaviour

Parameter IDs unchanged. State format increments to version 2 and appends `archetype` after the version-1 fields; a version-1 state restores all original controls and maps to archetype 0. Because this remains an unreleased sound-design prototype, old patches may not render sample-identically due to revised synthesis. Exact VST3 host state round-trip and dense automation require separate validation.

## Tests and thresholds (declared before final measurement)

`metallator_diversity_contract`: each pair of four generated archetypes must have 2-second **10ms RMS-envelope normalized cosine similarity below 0.96**, requiring structural temporal changes independent of gain or pitch shifting. Variation within one archetype retains envelope cosine ≥0.55 (initial broad preservation gate). That is a morphology gate, **not** a sonic-quality score; similarities in noise spectra may remain too high.

`metallator_seed_matrix`: across four rates (44.1 / 48 / 96 / 192 kHz), four engines, four archetypes and eight fixed seeds = **512 measured cases**. Every case requires finite output, <0.98 absolute sample ceiling, >0.02 peak for the first 0.65 seconds and L/R energy imbalance <3 dB. Very-long-duration and additional block-size/host tests remain pending.

Existing core-determinism, single IMPACT and stereo contract tests remain mandatory. Run Release-mode CTest locally before CI. Real-world audition and host validation are independent OPEN gates.

## CPU stress information, not a Realtime PASS

Standalone Linux test of 12 active voices, 64 frames per callback at 48 kHz over 2,800 block measurements per sound type recorded low p99 (~0.11–0.18 ms in this environment) but **multiple isolated max overruns beyond 1.333ms**, likely including host scheduler interference. This is an unpaced synthetic host-independent benchmark with no DAW or Windows workload and cannot establish hard realtime performance. Keep 125A realtime sign-off **NOT VERIFIED** pending paced reference tests, p95/p99/max/deadline data and native Windows/host testing.

## Honest limitations / remaining work

- The user must listen to the actual C++ offline WAVs before the new synthesis is considered musically useful.
- Some DRONE sound spectra and envelopes remain similar even where contact / resonance construction differs. A high audio difference is not proof of a credible industrial machine.
- Atonal synthesis does not mean noise-only; more convincing energy-transfer / interaction models are future work if these samples are insufficient.
- Existing VST3 plugin build is **not** updated or approved by this offline branch; GUI, in-plugin WAV export, state migration and automation, Steinberg validator and all 125A host/Editor lifecycle tests remain OPEN.
