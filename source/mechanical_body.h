#pragma once
// 125A Metallator — atonal mechanical percussion: IMPACT / PERC only.
// Design model is EMPIRICALLY TUNED, not an acoustic measurement of a real object.
// All coefficients are established at noteOn; render() is bounded, allocation-free,
// deterministic and contains neither trigonometric functions nor exponentials.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace MetallatorDSP {

class MechanicalBody final {
public:
    static constexpr int kModeCount = 48;
    static constexpr double kTau = 6.28318530717958647693;
    struct Stereo { double l{0}, r{0}; };

    // engine: IMPACT (alternative profiles) or PERC.
    void start(unsigned engine, unsigned shape, double rate, double size, double force,
               double chaos, double decay, uint32_t seed) noexcept {
        *this = {}; // reinitialize full object in-place; no allocations.
        engine_ = std::min(engine, 1u); shape_ = shape % 4;
        rate_ = std::clamp(rate, 8000.0, 384000.0);
        seed_ = seed ? seed : 0x9e3779b9u;
        // Preserve archetype-level contact timing under VARIATE: a separate
        // deterministic event stream is independent of modal microgeometry/noise.
        eventSeed_=0x263A92B1u ^ (engine_<<21) ^ (shape_<<11);
        force_ = std::clamp(force, 0.0, 1.0);
        chaos_ = std::clamp(chaos, 0.0, 1.0);
        size_ = std::clamp(size, 0.0, 1.0);
        decay_ = std::clamp(decay, 0.0, 1.0);
        lp120Coeff_ = pole(120.0); lp700Coeff_ = pole(700.0);
        lp3300Coeff_ = pole(3300.0); lp8500Coeff_ = pole(8500.0);
        // Architectural identity: body distribution, excitation mechanism and
        // attack/release morphology selected together, never a global pitch shift.
        constexpr double impactLength[] = {1.0, .42, 1.8, 1.25};
        constexpr double percLength[]   = {.12, .35, .22, .53};
        const double* lengths = engine_ == 0 ? impactLength : percLength;
        maxSamples_ = static_cast<uint64_t>(rate_ * (engine_ == 0 ? 7.0 : 4.0));
        const double attackRate = engine_==0 ? (shape_==0 ? 14.0 : shape_==1 ? 4.5 : shape_==2 ? 28.0 : 6.5) :
                                  (shape_==0 ? 62.0 : shape_==1 ? 30.0 : shape_==2 ? 25.0 : 18.0);
        transientStep_ = std::exp(-attackRate/rate_);
        // The modal body has deliberately different shape-specific band occupancy.
        // Unlike arbitrary base-frequency scaling, the frequency *distribution* changes.
        constexpr double lowBounds[2][4] = {
            {50, 350, 1700, 6900}, // IMPACT
            {130, 650, 1800, 9500} // PERC
        };
        double spectralBias[4][3] = {
            {.40, .22, .38},  // sheet snap: high metal flash
            {.63, .29, .08},  // block crush: heavy lower body
            {.20, .48, .32},  // braced cage: brighter resonant structure
            {.35, .43, .22}   // thin plate: modal ring
        };
        const double ratio = std::exp((.58 - size_) * 1.10);
        const double t60 = (engine_ == 0 ? 1.4 : .13) * (0.6 + 1.65 * decay_);
        for (int i = 0; i < kModeCount; ++i) {
            Mode& m = modes_[static_cast<std::size_t>(i)];
            const int band = i < 16 ? 0 : i < 32 ? 1 : 2;
            // Changing the number of modes per band changes material identity.
            const int shiftedBand = engine_ == 0 ?
                (shape_ == 0 ? (i < 7 ? 0 : i < 27 ? 1 : 2) :
                 shape_ == 1 ? (i < 27 ? 0 : i < 43 ? 1 : 2) :
                 shape_ == 2 ? (i < 9 ? 0 : i < 32 ? 1 : 2) : band) :
                (shape_ == 0 ? (i < 5 ? 0 : i < 20 ? 1 : 2) :
                 shape_ == 1 ? (i < 15 ? 0 : i < 31 ? 1 : 2) :
                 shape_ == 2 ? (i < 25 ? 0 : i < 42 ? 1 : 2) : band);
            const double lo = lowBounds[engine_][shiftedBand];
            const double hi = lowBounds[engine_][shiftedBand+1];
            const double frequency = std::clamp(std::exp(std::log(lo) + rand01() * std::log(hi/lo)) *
                ratio * (1.0 + (rand01()-.5)*chaos_*.08), 25.0, rate_*.43);
            const double phase = kTau * frequency / rate_;
            // A tightly braced massive block dissipates modal energy quickly;
            // a freely suspended sheet holds bending resonances substantially longer.
            // Shape is determined by structure (GENERATE), not by global pitch.
            const double contactDamping = engine_ == 0 && shape_ == 1 ? .32 :
                                          engine_ == 0 && shape_ == 3 ? 1.35 : 1.0;
            const double damping = t60 * (0.45 + 1.1*rand01()) * contactDamping;
            const double radius = std::exp(-6.907755278982137 / (rate_ * std::max(.012,damping)));
            m.a1 = 2*radius*std::cos(phase);
            m.a2 = -radius*radius;
            const double bandGain = engine_ == 0 ? spectralBias[shape_][shiftedBand] :
                (shape_ == 2 ? (shiftedBand == 0 ? .85 : .19) :
                 shape_ == 0 ? (shiftedBand == 2 ? .9 : .38) : .49);
            m.weight = bandGain * (0.45 + rand01() * .55) / std::sqrt(double(kModeCount));
            m.pan = (rand01() - 0.5) * .29;
            m.inputScale = (1.0 - radius) * 2.1; // constant-noise stationary energy scaling.
            // Impacts and PERC begin with modal initial *velocity*, not just noise.
            if (engine_ == 0 || (engine_ == 1 && shape_ != 1)) {
                const double a = (engine_ == 0 ? .27 : .038) * m.weight;
                const double theta = (rand01() - .5) * 1.0;
                m.previous = a * std::sin(theta-phase);
                m.current = a * std::sin(theta);
            }
        }
        // First event/particle counts and contact cadence are archetype-dependent,
        // not pitch-derived. A natural rattle is not a tempo-synced echo.
        countdown_ = 0;
        eventLevel_ = engine_ == 1 && shape_ == 1 ? 0.0 : .85;
        eventDecay_ = std::exp(-(engine_ == 1 ? 160.0 : 110.0) / rate_);
    }

    uint64_t maxSamples() const noexcept { return maxSamples_; }
    Stereo render() noexcept {
        const double t = static_cast<double>(age_) / rate_;
        const double noise = 2.0*rand01() - 1.0;
        l120_ = lp120Coeff_ * l120_ + (1.-lp120Coeff_) * noise;
        l700_ = lp700Coeff_ * l700_ + (1.-lp700Coeff_) * noise;
        l3300_ = lp3300Coeff_ * l3300_ + (1.-lp3300Coeff_) * noise;
        l8500_ = lp8500Coeff_ * l8500_ + (1.-lp8500Coeff_) * noise;
        const double body = l700_ - l120_;
        const double upper = l3300_ - l700_;
        const double grit = l8500_ - l3300_;
        const double high = noise - l8500_;
        double texture=0.0, mod=0.0, excite=0.0;
        // An event scheduler with bounded work and an explicit temporal contract.
        // Never uses MIDI key for pitch or timing in this atonal engine.
        eventLevel_ *= eventDecay_;
        if (engine_ == 0) {
            if (shape_ == 0) { // stressed steel sheet: an instantaneous broad crack
                mod = attack(t,.0006) * transient_;
                texture = .28*body + .58*upper + .41*grit + .06*high;
                excite = .26*mod*texture;
            } else if (shape_ == 1) { // heavy steel compression: low sustained buckling
                mod = attack(t,.010)*transient_;
                texture = 1.8*body + .35*upper + .025*grit;
                excite = .64*mod*texture;
            } else if (shape_ == 2) { // rigid steel cage: bright snap with hard rattle-free tail
                mod = attack(t,.001) * transient_;
                texture = .42*body + .85*upper + .16*grit;
                excite = .30*mod*texture;
            } else { // large suspended plate: swelling low-metal boom
                mod = attack(t,.018) * transient_;
                texture = 1.2*body + .45*upper + .07*grit;
                excite = .42*mod*texture;
            }
        } else if (engine_ == 1) {
            if (shape_ == 0) { // clipped industrial metal tick
                mod = transient_;
                texture = .28*body + .76*upper + .22*grit + .04*high;
                excite = .20*mod*texture;
            } else if (shape_ == 1) { // falling chain links: multiple small natural contacts
                if (!countdown_ && t < .38) {
                    eventLevel_ += (.40 + .60*randEvent()) * (1.0 - t/0.42);
                    countdown_ = static_cast<uint32_t>(rate_*(.009 + randEvent()*.026));
                }
                if (countdown_) --countdown_;
                mod = eventLevel_;
                texture = .24*body + .36*upper + .48*grit;
                excite = .24*texture*mod;
            } else if (shape_ == 2) { // heavy sheet click with sustained low shell ring
                mod = attack(t,.003)*transient_;
                texture = .82*body + .36*upper + .04*grit;
                excite = .24*texture*mod;
            } else { // serrated ratchet: fine impulses with slowing mechanical roll
                if (!countdown_ && t < .65) {
                    eventLevel_ += .25 + .30*randEvent();
                    countdown_ = static_cast<uint32_t>(rate_*(.018 + .040*t + .012*randEvent()));
                }
                if (countdown_) --countdown_;
                mod = eventLevel_ * std::max(0.0, 1.0 - t/0.72);
                texture = .22*body + .60*upper + .35*grit;
                excite = .30*texture*mod;
            }
        }
        // A suspended plate transfers energy into broad bending modes gradually
        // after the initial contact; the dry contact noise remains instantaneous.
        // This is an onset/bloom of the SAME impact, not an echo or retrigger.
        const double modalOnset = (engine_ == 0 && shape_ == 3) ? attack(t, .09) : 1.0;
        // Side channel is constructed from independently scattered modal pans,
        // without a second Mid/Side conversion downstream.
        double l=0.0, r=0.0;
        for(auto& m:modes_) {
            const double y = m.a1*m.current + m.a2*m.previous + m.inputScale*excite;
            m.previous = m.current;
            m.current = std::abs(y)<1e-28 ? 0.0 : y;
            // Structural modes form the audible body; direct noise is only contact.
            const double bodyBoost = engine_ == 0 ? 5.0 : 4.0;
            const double g = bodyBoost * modalOnset * m.weight * m.current;
            l += g*(1.0-m.pan);
            r += g*(1.0+m.pan);
        }
        const double direct = (engine_ == 0 ? .40 : .41) * texture * mod;
        // Fixed per-archetype gain calibration compensates distinct body mechanisms;
        // it does not normalize every hit in realtime or disguise mere seed changes.
        constexpr double impactGains[] = {1.0, 2.5, 6.3, 6.0};
        constexpr double percGains[]   = {1.9, 2.2, 1.15, 2.0};
        const double cal = engine_ == 0 ? impactGains[shape_] : percGains[shape_];
        const double global = cal * (engine_ == 0 ? 1.6 : 6.5) * force_;
        const double tail = std::clamp((double(maxSamples_-std::min(age_,maxSamples_))/rate_)/.045,0.0,1.0);
        ++age_;
        transient_ *= transientStep_;
        return {tail*global*(l+direct),tail*global*(r+direct)};
    }

private:
    struct Mode { double current{0}, previous{0},a1{0},a2{0},weight{0},pan{0},inputScale{0}; };
    static uint32_t prng(uint32_t& s) noexcept { s ^= s<<13; s ^= s>>17; s ^= s<<5; return s; }
    double rand01() noexcept { return double(prng(seed_))* (1.0/4294967295.0); }
    double randEvent() noexcept { return double(prng(eventSeed_))* (1.0/4294967295.0); }
    double pole(double f) const noexcept { return std::exp(-kTau*std::min(f,rate_*.46)/rate_); }
    static double attack(double t,double seconds) noexcept { return std::clamp(t/std::max(.0001,seconds),0.0,1.0); }
    std::array<Mode,kModeCount> modes_{};
    double rate_{44100},force_{.8},chaos_{.5},size_{.5},decay_{.5};
    double lp120Coeff_{0},lp700Coeff_{0},lp3300Coeff_{0},lp8500Coeff_{0};
    double l120_{0},l700_{0},l3300_{0},l8500_{0};
    double eventLevel_{0},eventDecay_{.999};
    double transient_{1.0},transientStep_{.999};
    uint32_t seed_{1},eventSeed_{0x263A92B1u},countdown_{0};
    uint64_t age_{0},maxSamples_{1};
    unsigned engine_{0},shape_{0};
};
} // namespace MetallatorDSP
