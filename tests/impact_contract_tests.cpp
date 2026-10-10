// Contract-based tests for 125A Metallator: one atonal steel strike, no echoes.
// Unlike assert(), REQUIRE executes in both Debug and Release CI builds.
#include "metal_synth.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace MetallatorDSP;
[[noreturn]] void fail(const char* desc, int line) {
    std::cerr << "IMPACT CONTRACT FAILED line " << line << ": " << desc << '\n';
    std::exit(1);
}
#define REQUIRE(expr) do { if (!(expr)) fail(#expr, __LINE__); } while(false)

struct Analysis {double earlyDb{}, bodyDb{}, lateDb{}, maxPeak{}, dc{}, avgStereoDifference{};};
static double levelDb(double sum, std::size_t count) {
    return 20.0 * std::log10(std::sqrt(sum / std::max<std::size_t>(count,1)) + 1e-20);
}
static Analysis measure(double sr, uint32_t seed, double force, double decay, int key=60) {
    Synth synth;
    synth.setSampleRate(sr);
    Patch p; p.engine=Engine::Impact;p.seed=seed;p.force=force;p.decay=decay;
    synth.setPatch(p);
    synth.noteOn(key, 0.9, 11);
    const int n = int(sr * 2.0);
    double early = 0, body = 0, late = 0, peak = 0, dc = 0, stereo = 0;
    int ne=0,nb=0,nl=0;
    for (int i = 0; i < n; ++i) {
        if (i == int(sr * 0.1)) synth.noteOff(key,11); // must NOT cut percussive steel body
        const auto x = synth.renderFrame();
        REQUIRE(std::isfinite(x[0]) && std::isfinite(x[1]));
        REQUIRE(std::abs(x[0]) <= 0.98000001 && std::abs(x[1]) <= 0.98000001);
        peak = std::max({peak,std::abs(x[0]),std::abs(x[1])});
        dc += x[0]; stereo += std::abs(x[0]-x[1]);
        const double energy = .5 * (x[0]*x[0]+x[1]*x[1]);
        if(i < int(sr*.05)) {early += energy; ++ne;}
        if(i >= int(sr*.25) && i < int(sr*.75)) {body += energy; ++nb;}
        if(i >= int(sr*1.) && i < int(sr*1.5)) {late += energy; ++nl;}
    }
    return {levelDb(early,ne),levelDb(body,nb),levelDb(late,nl),peak,dc/n,stereo/n};
}

int main() {
    for (double sr : {44100.,48000.,96000.,192000.}) {
        auto q = measure(sr, 307896358u, .8, .55);
        std::cout << sr << " Hz strike early=" << q.earlyDb << " dBFS, body="
                  << q.bodyDb << " dBFS, tail=" << q.lateDb << " dBFS, peak="
                  << q.maxPeak << "\n";
        // User feedback: V1 body at .25-.75s was ~-65 dBFS (much too faint).
        // Threshold is declared before CI and holds on all supported common rates.
        REQUIRE(q.bodyDb > -43.0);
        REQUIRE(q.bodyDb < q.earlyDb + 5.0); // pressure/body must not spike later as an echo
        REQUIRE(q.lateDb < q.bodyDb + 6.0); // decay; no secondary triggered impacts
        REQUIRE(q.maxPeak < .97); // one voice must have usable peak headroom
        REQUIRE(std::abs(q.dc) < .02);
        REQUIRE(q.avgStereoDifference > .00001);
        auto weak=measure(sr, 307896358u, .1, .55);
        REQUIRE(weak.maxPeak < q.maxPeak);
    }
    // In an atonal engine, MIDI pitch is a trigger only (seed & velocity unchanged).
    const auto a=measure(48000,307896358u,.8,.55,36);
    const auto b=measure(48000,307896358u,.8,.55,96);
    REQUIRE(a.earlyDb==b.earlyDb && a.bodyDb==b.bodyDb && a.lateDb==b.lateDb);
    // GENERATE changes the seeded body geometry, not only the output noise sample.
    auto one=measure(48000,307896358u,.8,.55);
    auto two=measure(48000,nextGeneration(307896358u),.8,.55);
    REQUIRE(std::abs(one.bodyDb-two.bodyDb) > .2 || std::abs(one.earlyDb-two.earlyDb) > .2);
    // FORCE=0 has no residual strike noise or hidden ringing.
    { Synth off;Patch op;op.engine=Engine::Impact;op.force=0.0;off.setPatch(op);
      off.noteOn(60,1.0);for(int i=0;i<4800;++i){const auto z=off.renderFrame();REQUIRE(z[0]==0.0 && z[1]==0.0);} }
    // Changing the patch via level=0 silences the output, no hidden residual source.
    Synth muted;Patch p;p.engine=Engine::Impact;p.level=0;muted.setPatch(p);muted.noteOn(64,1);
    for(int i=0;i<4800;++i){auto s=muted.renderFrame();REQUIRE(s[0]==0.0&&s[1]==0.0);}
    std::cout << "IMPACT single-strike contract PASS\n";
}
