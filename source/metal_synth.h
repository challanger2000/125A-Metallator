#pragma once

// 125A Metallator: self-contained deterministic metal synthesis core.
// No SDK dependency; no allocations, locks, file I/O or calls to the host during render.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace MetallatorDSP {

enum class Engine : unsigned { Impact = 0, Perc = 1, Friction = 2, Drone = 3 };

struct Patch {
    Engine engine {Engine::Impact};
    double size {0.75};
    double force {0.80};
    double chaos {0.55};
    double decay {0.55};
    double keyTrack {0.0}; // Explicitly optional; default atonal.
    double level {0.68};
    uint32_t seed {0x125A2026u};
};

inline double unit(double x) noexcept {
    return std::isfinite(x) ? std::clamp(x, 0.0, 1.0) : 0.0;
}
inline uint32_t nextRandom(uint32_t& s) noexcept {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    if (s == 0) s = 0x6D2B79F5u;
    return s;
}
inline double randomSigned(uint32_t& s) noexcept {
    return static_cast<double>(nextRandom(s)) * (2.0 / 4294967295.0) - 1.0;
}
inline uint32_t scrambled(uint32_t x) noexcept {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15;
    x *= 0x846ca68bu; x ^= x >> 16;
    return x == 0 ? 0x9e3779b9u : x;
}

class Synth final {
public:
    static constexpr int kVoices = 12;
    static constexpr int kModes = 14;

    void setSampleRate(double sampleRate) noexcept {
        rate_ = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 44100.0;
        reset();
    }
    double sampleRate() const noexcept { return rate_; }

    void setPatch(Patch p) noexcept {
        p.size = unit(p.size); p.force = unit(p.force); p.chaos = unit(p.chaos);
        p.decay = unit(p.decay); p.keyTrack = unit(p.keyTrack); p.level = unit(p.level);
        if (static_cast<unsigned>(p.engine) > 3) p.engine = Engine::Impact;
        patch_ = p;
    }
    const Patch& patch() const noexcept { return patch_; }
    void reset() noexcept {
        voices_ = {};
        voiceIndex_ = 0;
        hitIndex_ = 0;
    }
    void noteOn(int key, double velocity, int32_t noteId = -1) noexcept {
        if (key < 0 || key > 127 || !(velocity > 0.0)) return;
        const int target = findVoice();
        Voice& v = voices_[static_cast<std::size_t>(target)];
        v = {};
        v.active = true;
        v.key = key;
        v.noteId = noteId;
        v.mode = patch_.engine;
        v.velocity = unit(velocity);
        v.rng = scrambled(patch_.seed ^ (0x9e3779b9u * (++hitIndex_)));
        // One deterministic draw per parameter; changes in synthesis do not change patch identity.
        v.charA = randomSigned(v.rng);
        const double size = patch_.size;
        const double chaos = patch_.chaos;
        const bool drone = v.mode == Engine::Drone;
        const double keyRatio = std::pow(2.0, (double(key) - 60.0) / 12.0);
        const double tracked = 1.0 + patch_.keyTrack * (keyRatio - 1.0);
        const double base = (v.mode == Engine::Perc ? 470.0 : v.mode == Engine::Friction ? 185.0 : drone ? 80.0 : 125.0)
                            * std::pow(2.0, -(drone ? 1.5 : 2.0) * size) * (drone ? tracked : 1.0);
        const double baseTime = v.mode == Engine::Perc ? 0.045 : v.mode == Engine::Impact ? 0.17 :
                                v.mode == Engine::Friction ? 0.22 : 1.3;
        const double timeFactor = v.mode == Engine::Perc ? 8.0 : drone ? 18.0 : 10.0;
        const double t60 = baseTime * std::pow(timeFactor, patch_.decay) * (0.8 + 0.2 * (v.charA + 1.0));
        v.maxAge = static_cast<uint64_t>(rate_ * (drone ? 30.0 : std::min(9.0, t60 * 4.0 + 0.2)));
        v.duration = v.mode == Engine::Perc ? 0.0015 + 0.014 * patch_.decay :
                     v.mode == Engine::Impact ? 0.003 + 0.012 * patch_.force :
                     v.mode == Engine::Friction ? 0.055 + 0.80 * patch_.decay : 1.0;
        const double spread = (0.02 + 0.24 * chaos);
        for (int i = 0; i < kModes; ++i) {
            auto& m = v.modes[static_cast<std::size_t>(i)];
            const double index = double(i);
            // Deliberately inharmonic distributions, with a family-specific spectral density.
            const double ratio = v.mode == Engine::Perc ? (1.0 + 0.58 * index + 0.18 * index * index) :
                                 v.mode == Engine::Friction ? (1.0 + 0.96 * index + 0.13 * index * index) :
                                 v.mode == Engine::Drone ? (1.0 + 0.81 * index + 0.05 * index * index) :
                                 (1.0 + 1.16 * index + 0.10 * index * index);
            const double hz = std::clamp(base * ratio * (1.0 + spread * randomSigned(v.rng)), 22.0, 0.42 * rate_);
            const double damping = t60 * std::pow(0.91, index) * (0.85 + 0.3 * (randomSigned(v.rng) + 1.0) / 2.0);
            const double radius = std::exp(-6.907755279 / (rate_ * std::max(0.012, damping)));
            m.a1 = 2.0 * radius * std::cos(6.283185307179586 * hz / rate_);
            m.a2 = -radius * radius;
            m.inGain = std::max(1.e-7, 1.0 - radius);
            const double tilt = v.mode == Engine::Perc ? 0.15 + 0.9 * index / kModes :
                                v.mode == Engine::Drone ? 1.3 - 0.065 * index : 1.0 - 0.035 * index;
            m.outGain = tilt * (0.48 + 0.52 * unit(0.5 + 0.5 * randomSigned(v.rng)));
            m.sideGain = 0.065 * chaos * (i % 2 == 0 ? 1.0 : -1.0);
        }
    }
    void noteOff(int key, int32_t noteId = -1) noexcept {
        for (auto& v : voices_) {
            if (v.active && v.key == key && (noteId < 0 || v.noteId == noteId))
                v.released = true;
        }
    }
    void allNotesOff() noexcept { for (auto& v : voices_) v.released = true; }
    int activeVoices() const noexcept { int n = 0; for (const auto& v : voices_) n += int(v.active); return n; }

    std::array<double, 2> renderFrame() noexcept {
        double mid = 0.0, side = 0.0;
        for (auto& v : voices_) {
            if (!v.active) continue;
            const auto frame = renderVoice(v);
            mid += frame[0]; side += frame[1];
        }
        const double gain = 0.26 * patch_.level;
        const double l = (mid + side) * gain;
        const double r = (mid - side) * gain;
        // Bounds unexpected transient combinations without changing the nominal linear signal path.
        return {std::clamp(l, -0.98, 0.98), std::clamp(r, -0.98, 0.98)};
    }

private:
    struct Mode {
        double y1 {0}, y2 {0};
        double a1 {0}, a2 {0}, inGain {0}, outGain {0}, sideGain {0};
    };
    struct Voice {
        bool active {false}, released {false};
        int key {0}, noteId {-1};
        Engine mode {Engine::Impact};
        uint32_t rng {1};
        uint64_t age {0}, maxAge {0};
        double velocity {1}, duration {0};
        double charA {0};
        double prevNoise {0}, noiseLow {0}, scrapeMemory {0};
        double releaseGain {1};
        std::array<Mode, kModes> modes {};
    };
    int findVoice() noexcept {
        for (int i = 0; i < kVoices; ++i) if (!voices_[i].active) return i;
        // Bounded round-robin stealing; no allocations.
        const int index = int(voiceIndex_ % kVoices);
        ++voiceIndex_;
        return index;
    }
    std::array<double, 2> renderVoice(Voice& v) noexcept {
        const double time = double(v.age) / rate_;
        const double noise = randomSigned(v.rng);
        const double high = noise - v.prevNoise;
        v.prevNoise = noise;
        v.noiseLow = 0.985 * v.noiseLow + 0.015 * noise;
        const double low = v.noiseLow;
        const double force = patch_.force;
        const double chaos = patch_.chaos;
        double excite = 0.0;
        double direct = 0.0;
        if (v.mode == Engine::Impact) {
            const double burst = std::exp(-time * (500.0 - 290.0 * force));
            const double recoilTime = 0.026 + 0.032 * unit(0.5 + 0.5 * v.charA);
            const double recoil = time >= recoilTime ? (0.05 + 0.38 * chaos) *
                std::exp(-(time - recoilTime) * 240.0) : 0.0;
            excite = (burst + recoil) * (0.38 * noise + 0.45 * high);
            direct = 0.30 * (burst + recoil) * high;
        } else if (v.mode == Engine::Perc) {
            const double envelope = std::exp(-time * (24.0 + 86.0 * (1.0 - patch_.decay)));
            const double tick = std::exp(-time * (350.0 + 280.0 * (1.0 - force)));
            excite = 0.75 * envelope * high;
            direct = 0.65 * envelope * (high - 0.15 * low) + 0.35 * tick * high;
        } else if (v.mode == Engine::Friction) {
            const double on = std::min(1.0, time * 85.0);
            const double motion = time < v.duration ? std::min(1.0, (v.duration - time) * 90.0) : 0.0;
            // Stick-slip surrogate: nonlinear stored friction stress with stochastically changing contact.
            const double drive = (0.32 + 0.62 * force) * (noise * (0.2 + 0.3 * chaos) + 0.5 * std::sin(6.2831853 * (39.0 + 97.0 * chaos) * time));
            v.scrapeMemory = std::clamp(0.962 * v.scrapeMemory + 0.038 * drive, -1.0, 1.0);
            const double slip = drive - v.scrapeMemory;
            excite = on * motion * (slip + 0.2 * high);
            direct = on * motion * (0.28 * slip + 0.12 * high);
        } else { // DRONE: high-density aperiodic excitation, not a forced musical oscillator.
            const double ramp = std::min(1.0, time * (1.2 + 2.0 * force));
            const double undulation = 0.80 + 0.20 * std::sin(6.2831853 * (0.3 + 1.7 * chaos) * time);
            excite = 0.28 * ramp * undulation * (noise + 0.6 * low);
            direct = 0.15 * ramp * low;
        }
        if (v.released && (v.mode == Engine::Drone || v.mode == Engine::Friction))
            v.releaseGain *= std::exp(-6.907755279 / (rate_ * (v.mode == Engine::Drone ? 0.18 + 1.2 * patch_.decay : 0.02 + 0.12 * patch_.decay)));
        // Transient/resonance gains are dimensionless, and bounded across sample rates.
        double resonant = 0.0, stereo = 0.0;
        for (auto& m : v.modes) {
            const double y = m.a1 * m.y1 + m.a2 * m.y2 + m.inGain * excite;
            m.y2 = m.y1;
            m.y1 = std::abs(y) < 1e-28 ? 0.0 : y;
            const double component = m.outGain * m.y1;
            resonant += component;
            stereo += component * m.sideGain;
        }
        const double blend = v.mode == Engine::Perc ? 0.40 : v.mode == Engine::Drone ? 1.3 : 0.85;
        const double familyGain = v.mode == Engine::Impact ? 3.7 :
                                  v.mode == Engine::Perc ? 2.0 :
                                  v.mode == Engine::Friction ? 3.0 : 2.0;
        const double amp = familyGain * v.velocity * (0.25 + 0.75 * force) * v.releaseGain;
        const double value = (blend * resonant + direct) * amp;
        const double side = stereo * amp;
        ++v.age;
        if (v.age >= v.maxAge || (v.released && v.releaseGain < 1.e-5)) v.active = false;
        return {std::isfinite(value) ? value : 0.0, std::isfinite(side) ? side : 0.0};
    }
    Patch patch_ {};
    double rate_ {44100.0};
    std::array<Voice, kVoices> voices_ {};
    uint32_t hitIndex_ {0};
    uint32_t voiceIndex_ {0};
};

inline uint32_t nextGeneration(uint32_t current) noexcept {
    return scrambled(current + 0x9e3779b9u);
}

} // namespace MetallatorDSP
