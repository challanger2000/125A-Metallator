// 125A Metallator Stereo Contract: absolute channel balance for a centered
// one-shot impact, without forcing the stochastic metal body to mono.
// Tests the complete Synth stereo signal path, not ImpactBody in isolation.
#include "metal_synth.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace MetallatorDSP;
[[noreturn]] static void fail(const char* expression, int line, double value = 0) {
    std::cerr << "STEREO CONTRACT FAILED line " << line << ": " << expression
              << " (value=" << value << ")\n";
    std::exit(1);
}
#define REQUIRE(x) do { if (!(x)) fail(#x, __LINE__); } while(false)

struct StereoLevel { double imbalanceDb; double peak; double width; };
static StereoLevel measure(double sampleRate, Engine engine, uint32_t seed) {
    Synth synth;
    synth.setSampleRate(sampleRate);
    Patch patch;
    patch.engine = engine;
    patch.seed = seed;
    synth.setPatch(patch);
    synth.noteOn(60, 0.9, 13);
    double l2 = 0, r2 = 0, s2 = 0, m2 = 0, peak = 0;
    const int total = static_cast<int>(sampleRate * 2.0);
    for (int i=0; i<total; ++i) {
        if (i == static_cast<int>(0.75 * sampleRate)) synth.noteOff(60, 13);
        const auto v = synth.renderFrame();
        REQUIRE(std::isfinite(v[0]) && std::isfinite(v[1]));
        l2 += v[0] * v[0];
        r2 += v[1] * v[1];
        const double side = .5 * (v[0] - v[1]);
        const double mid = .5 * (v[0] + v[1]);
        s2 += side * side;
        m2 += mid * mid;
        peak = std::max({peak, std::abs(v[0]), std::abs(v[1])});
    }
    REQUIRE(l2 > 1e-6 && r2 > 1e-6);
    const double imbalance = 10.0 * std::log10(l2 / r2);
    const double width = std::sqrt(s2 / std::max(m2, 1e-30));
    return {imbalance, peak, width};
}
int main() {
    const uint32_t seeds[] = {307896358u, 0x125A2026u, 0x1234u, 0x12345678u};
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0}) {
        for (uint32_t seed : seeds) {
            const auto impact = measure(rate, Engine::Impact, seed);
            std::cout << "IMPACT stereo " << rate << " seed=" << seed
                      << " L-R=" << impact.imbalanceDb << " dB, width="
                      << impact.width << ", peak=" << impact.peak << '\n';
            // Intentionally centered mono-compatible strike with natural stereo spread.
            // 3 dB is a broad tolerance chosen as a product acceptance bound, not a
            // psychoacoustic absolute; the previous defect measured +17 to +20 dB.
            if (std::abs(impact.imbalanceDb) > 3.0) {
                fail("IMPACT ABS(L-R) <= 3 dB", __LINE__, impact.imbalanceDb);
            }
            REQUIRE(impact.width < 0.75); // no almost-opposing left/right collapse
            REQUIRE(impact.peak <= .98);
        }
        // Other engines use mid/side internally and must retain correct stereo output.
        for (auto family : {Engine::Perc, Engine::Friction, Engine::Drone}) {
            const auto other = measure(rate, family, seeds[0]);
            if (std::abs(other.imbalanceDb) > 3.0)
                fail("NONIMPACT ABS(L-R) <= 3 dB", __LINE__, other.imbalanceDb);
        }
    }
    std::cout << "Stereo balance contract PASS\n";
}
