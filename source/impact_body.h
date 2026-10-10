#pragma once
// Independent physical-inspired, empirically tuned, atonal IMPACT exciter.
// The single initial contact excites a dense free-decaying inharmonic steel body.
// NO echoes, rebound triggers, additional impacts, keyed pitch, or audio files.
// Offline reference: 125A_Metallator_Offline_Experiments/generate.py example1()
// Retains the relevant THREE resonance groups, pressure and contact texture;
// intentionally omits all the secondary collision/debris events and room echoes.
// Run-time: fixed storage, bounded loops, no allocations or I/O.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace MetallatorDSP {

class ImpactBody final {
public:
    static constexpr int kModeCount = 84;
    static constexpr double kPi2 = 6.28318530717958647693;
    struct Stereo { double l {0.0}; double r {0.0}; };

    void strike(double sr, double size, double force, double chaos,
                double decay, uint32_t seed) noexcept {
        rate_ = std::clamp(sr, 8000.0, 384000.0);
        age_ = 0;
        pressureRise_ = pressureDecay_ = contactRise_ = contactDecay_ = 1.0;
        pressureRiseStep_ = std::exp(-90.0 / rate_);
        pressureDecayStep_ = std::exp(-1.55 / rate_);
        contactRiseStep_ = std::exp(-1400.0 / rate_);
        contactDecayStep_ = std::exp(-9.0 / rate_);
        seed_ = seed ? seed : 0x9e3779b9u;
        low300_ = low39_ = low7000_ = low600_ = low13000_ = low2600_ = 0.0;
        // One-pole LP difference creates finite bands, no colored noise pre-rendered.
        a300_ = onePole(300.0); a39_ = onePole(39.0);
        a7000_ = onePole(7000.0); a600_ = onePole(600.0);
        a13000_ = onePole(13000.0); a2600_ = onePole(2600.0);
        // Seed selects a *geometry* (beam / sheet / cage) in addition to modal scatter;
        // GENERATE can therefore produce different structures, not just different noise.
        const unsigned geometry = rnd(seed_) % 3u;
        const double geometricScale = geometry == 0 ? 0.82 : geometry == 1 ? 1.0 : 1.17;
        const double sizeScale = std::pow(2.0, (0.75 - size) * 1.3);
        const double decayScale = 0.40 + 0.92 * decay;
        // Modal group ranges & strengths from the independent, user-preferred steel-impact
        // experiment. Values are empirically tuned, not claimed measurements of real steel.
        constexpr int groupCount[3] = {16, 33, 35};
        constexpr double lowHz[3] = {55.0, 220.0, 1900.0};
        constexpr double highHz[3] = {220.0, 1900.0, 8500.0};
        constexpr double minT60[3] = {2.0, 1.2, 0.24};
        constexpr double maxT60[3] = {5.5, 4.0, 1.6};
        constexpr double groupGain[3] = {1.85, 1.15, 0.74};
        int index = 0;
        const double maxHz = std::min(10000.0, rate_ * 0.43);
        for (int group = 0; group < 3; ++group) {
            const double weight = groupGain[group] / std::sqrt(double(groupCount[group]));
            for (int j = 0; j < groupCount[group]; ++j, ++index) {
                Mode& m = modes_[static_cast<std::size_t>(index)];
                const double frequency = std::clamp(
                    std::exp(std::log(lowHz[group]) + rand01() * std::log(highHz[group] / lowHz[group]))
                    * geometricScale * sizeScale * (1.0 + (rand01() - 0.5) * chaos * 0.12),
                    30.0, maxHz);
                const double t60 = (minT60[group] + rand01() * (maxT60[group] - minT60[group])) * decayScale;
                const double omega = kPi2 * frequency / rate_;
                const double radius = std::exp(-6.907755278982137 / (rate_ * t60));
                m.coef = 2.0 * radius * std::cos(omega);
                m.radiusSq = radius * radius;
                const double phase = (rand01() - 0.5) * 0.64;
                const double amplitude = weight * (0.52 + 0.48 * rand01());
                m.previous = amplitude * std::sin(phase - omega);
                m.current = amplitude * std::sin(phase);
                m.pan = (rand01() - 0.5) * (0.35 + 0.9 * chaos);
            }
        }
        // Most steel spectra are stochastic and atonal; keep no fundamental/key root.
        // FORCE=0 is genuinely silent; the default 80% gain is preserved.
        const double forceResponse = (force + .2 * force * force) / (.8 + .2 * .8 * .8);
        shockGain_ = forceResponse * (0.24 + 0.75 * force) * (0.75 + 0.25 * (1.0 - size));
        biteGain_ = forceResponse * (0.16 + 0.72 * force) * (0.65 + 0.35 * chaos);
        ringGain_ = forceResponse * (0.32 + 0.54 * force) * (0.75 + 0.25 * (1.0 - chaos));
        // One natural tail, not a synthetic cluster of secondary impact events.
        maximumSamples_ = static_cast<uint64_t>(rate_ * std::clamp(4.0 + 4.5 * decay, 4.0, 8.5));
    }

    uint64_t maxSamples() const noexcept { return maximumSamples_; }
    Stereo render() noexcept {
        const double t = double(age_) / rate_;
        // Fixed-coefficient oscillator recurrences: no sine or exponentials per mode/sample.
        // All modes start on the SAME strike; no timed re-triggers.
        double left = 0.0, right = 0.0;
        for (Mode& m : modes_) {
            const double s = m.coef * m.current - m.radiusSq * m.previous;
            m.previous = m.current;
            m.current = std::abs(s) < 1e-28 ? 0.0 : s;
            left += s * (0.70 - 0.36 * m.pan);
            right += s * (0.70 + 0.36 * m.pan);
        }
        // Three contact bands, generated independently. NO late strikes or room taps.
        const double n1 = randomSigned();
        const double n2 = randomSigned();
        const double n3 = randomSigned();
        const double lowBand = filter(n1, low300_, a300_) - filter(n1, low39_, a39_);
        const double midBand = filter(n2, low7000_, a7000_) - filter(n2, low600_, a600_);
        const double highBand = filter(n3, low13000_, a13000_) - filter(n3, low2600_, a2600_);
        const double pressureEnvelope = (1.0 - pressureRise_) * pressureDecay_;
        const double contactEnvelope = (1.0 - contactRise_) * contactDecay_;
        pressureRise_ *= pressureRiseStep_;
        pressureDecay_ *= pressureDecayStep_;
        contactRise_ *= contactRiseStep_;
        contactDecay_ *= contactDecayStep_;
        const double contact = (0.72 * midBand + 0.20 * highBand) * biteGain_ * contactEnvelope;
        const double pressure = 0.75 * lowBand * shockGain_ * pressureEnvelope;
        const double onset = std::min(1.0, t * 3000.0);
        // Gradual final tail protection; no abrupt cutoff at an arbitrary block boundary.
        const double duration = double(maximumSamples_) / rate_;
        const double remaining = duration - t;
        const double tail = std::clamp(remaining / 0.06, 0.0, 1.0);
        ++age_;
        return {tail * (ringGain_ * left * onset + 0.70 * (pressure + contact)),
                tail * (ringGain_ * right * onset + 0.70 * (pressure + contact))};
    }

private:
    struct Mode {
        double current {0.0};
        double previous {0.0};
        double coef {0.0};
        double radiusSq {0.0};
        double pan {0.0};
    };
    static uint32_t rnd(uint32_t& s) noexcept {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return s;
    }
    double rand01() noexcept { return double(rnd(seed_)) / 4294967295.0; }
    double randomSigned() noexcept { return 2.0 * rand01() - 1.0; }
    double onePole(double hz) const noexcept { return std::exp(-kPi2 * std::min(hz, rate_ * 0.46) / rate_); }
    static double filter(double x, double& state, double a) noexcept {
        state = a * state + (1.0 - a) * x;
        return state;
    }
    std::array<Mode, kModeCount> modes_ {};
    double rate_ {44100.0}, a300_ {0}, a39_ {0}, a7000_ {0}, a600_ {0}, a13000_ {0}, a2600_ {0};
    double low300_ {0}, low39_ {0}, low7000_ {0}, low600_ {0}, low13000_ {0}, low2600_ {0};
    double ringGain_ {0}, shockGain_ {0}, biteGain_ {0};
    double pressureRise_ {1.0}, pressureDecay_ {1.0}, contactRise_ {1.0}, contactDecay_ {1.0};
    double pressureRiseStep_ {1.0}, pressureDecayStep_ {1.0};
    double contactRiseStep_ {1.0}, contactDecayStep_ {1.0};
    uint32_t seed_ {1};
    uint64_t age_ {0}, maximumSamples_ {0};
};

} // namespace MetallatorDSP
