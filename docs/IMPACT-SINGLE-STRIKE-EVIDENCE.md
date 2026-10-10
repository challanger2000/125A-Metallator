# IMPACT single steel strike — v1.0.0 development QA

## User-observed failure and goal

Old IMPACT was a short, low-frequency, wooden/bathtub-like knock. The original
independent offline example `01_Stahltraeger_Kollision.wav` was preferred,
except for its unnecessary echo-like follow-up impacts. The desired DSP is ONE
massive non-pitched metallic impact, natural resonant tail, no percussion loop,
no delayed retrigger events, no tempo sync.

## Reference and method

Source: user-approved direction from `125A_Metallator_Offline_Experiments/generate.py`,
function `example1` / `sine_modes(..., kind='massive')`. The Python file itself
is not embedded in this product. Relevant algorithmic principles:

- A one-time mechanical excitation launches inharmonic resonant modes,
  rather than a single keyed oscillator.
- Pole radius for 60 dB decay: `r = exp(-ln(1000) / (sr * T60))`,
  with discrete stable mode recurrence `y[n] = 2*r*cos(w)*y[n-1] - r*r*y[n-2]`.
- 16 low-band, 33 mid-band and 35 upper-band modes, seeded log-distributed
  frequencies and seed-derived structural geometry.
- Three noise/contact bands for short attack (low pressure, mid abrasion,
  high spark); contact and pressure envelopes are stable recursive exponentials.
- No extra event scheduler, echo, debris impact, room-tap or MIDI key transpose.
- C1 continuous safety ceiling remains linear through +/-0.70, asymptotic
  bound +/-0.98. Used only as last-resort polyphony guard; not a mastering effect.

Mode distribution, source spectrum, amplitudes and pitch drift are **EMPIRICALLY
TUNED** sound design. This is NOT a validated physical model of an actual steel
beam. The signal processing derives from known stable second-order modal
recursions and exponential envelopes, not from inferred hardware material data.

## Local comparison — one-note 44,100 Hz reference, identical sample format

| Time region | Previous IMPACT RMS | New IMPACT RMS |
|---|---:|---:|
| 0–50 ms | −27.9 dBFS | around −18 dBFS |
| 250–750 ms | −64.8 dBFS | around −28.5 dBFS |
| 750–1500 ms | −111.1 dBFS | around −39 dBFS |

The measurement is **not level-matched** and thus establishes retained body
energy, NOT perceptual superiority. A controlled level-matched listening test
and real DAW validation remain OPEN.

## QA completed locally

- GCC 14/CMake Release, with `-DNDEBUG`, two CTest targets passing;
  all assertions are unconditional `CHECK`/`REQUIRE`, not C++ `assert`.
- `impact_contract_tests` evaluates 44.1/48/96/192 kHz, retained body,
  tail decay, deterministic geometry variation, FORCE off, stereo content,
  MIDI trigger key independence, finite bounds.
- First single-voice reference seed WAV peak: approximately 0.74 linear.
- Diagnostic **unpaced** 64-sample / 48-kHz CPU on a Linux container:
  1 voice p99 ~23 us, 4 voices ~62 us, 12 voices ~171 us; scheduler
  excursions occurred (including a 12-voice max exceeding the 1.333ms
  deadline). These figures are NOT a Windows or paced-real-time PASS.
- At extreme 12-voice layering, the output uses a continuous soft ceiling,
  not the previous flat hard-clipped output.

## OPEN gates

Steinberg VST3 Validator; 125A host tester; Windows Studio One test; audio
interface pacing under dense MIDI chords; measurable spectral variation;
psychoacoustically level-matched comparison; user listening sign-off;
custom VSTGUI; in-plugin file export; 100/150% GUI and editor lifecycle.

Reference filenames and exact seed values are delivered with the WAV audition
ZIP; no reference data should be regenerated to merely make failing tests green.
