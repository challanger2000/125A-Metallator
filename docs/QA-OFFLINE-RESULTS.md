> **HISTORICAL — previous four-engine sound-design research, not the current product specification.** See [FOCUS-2026-10-10.md](FOCUS-2026-10-10.md).

# Local evidence — Metallator structural sound design

Date: 2026-10-10. **Pre-release research ONLY; no Windows VST3 build triggered for this branch.**

## Scope and commands

Local host: Linux container, g++ C++20 optimized Release; CPU differs from user target and GitHub Windows worker.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMETALLATOR_BUILD_VST3=OFF -DBUILD_TESTING=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/metallator_render impact impact0.wav 307896358 0
./build/metallator_render impact impact1.wav 307896358 1
```

Five CTest suites: core deterministic/atonal/event safety, impact single strike, stereo balance, structural diversity/variation, 512-case seed/rate matrix. **5/5 PASS locally.**

All 16 archetypes rendered as independent WAV files from the **same C++ Synth class** used by the VST3 source (not the old isolated Python examples). Renderer file encoding 44.1 kHz / stereo / 24-bit integer PCM.

## Structural temporal-envelope test

No per-profile gain compensation is used to create audio files. For analysis only, each two-second waveform is converted into 10-ms RMS bins and normalized to unit Euclidean length; this makes the structural comparison indifferent to loudness changes. The pairwise threshold of <0.96 was specified in `tests/diversity_contract_tests.cpp` before final QA.

Observed median temporal-envelope cosine across six profile pairs (one fixed seed, lower = different shapes):

| Engine | Previous 16-seed median (same shape) | New four-archetype median |
|---|---:|---:|
| IMPACT | 0.966 | ~0.85 |
| PERC | 1.000 | ~0.70 |
| FRICTION | 0.995 | ~0.70 |
| DRONE | 0.937 | ~0.89 |

The before/after cohorts are not identical experiments (seeds within one sound versus deliberately changed sound structures). The values demonstrate the new architecture's morphology changes; they do **not** provide a perceptual quality score or prove a specific subjective improvement.

Additional review measures broadband spectral-shape correlation allowing log-frequency shift. Some DRONE pairs still show correlation >0.95; therefore **distinct-sounding mechanical machines are not yet established**.

## Seed matrix

For each of four engines × four archetypes × eight frozen seeds × 44.1/48/96/192 kHz (512 cases), check initial 650 ms for finite values, absolute peaks above 0.02 and below 0.98, and L/R level balance within 3 dB. **512/512 pass** locally. These are deliberately modest sanity guards, not claims of mastering headroom or industrial believability.

## Profiling warning

The standalone 64-sample/48 kHz benchmark exercised all 12 voices and recorded p95/p99/max. It recorded isolated overruns of the nominal 1.333 ms block deadline in this environment in several PERC/DRONE profiles. The test was run **without paced host-like scheduling**, so these results are diagnostics only. Realtime performance **NOT VERIFIED**; native Windows paced measurements remain mandatory.

## Open QA gates

Steinberg Validator, VST3 state migration V1→V2, host automation and parameter updates, editor lifecycle, sample-accurate host events, DAW audio routing, full CPU stress, real musical audition and sound approval **NOT YET VERIFIED**. No release claim.
