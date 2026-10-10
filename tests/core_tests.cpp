#include "metal_synth.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace MetallatorDSP;
// Unlike assert(...), CHECK(...) is still active in Release/CI.
[[noreturn]] void failed(const char* expression, int line) {
    std::cerr << "DSP CHECK FAILED line " << line << ": " << expression << "\n";
    std::exit(1);
}
#define CHECK(expr) do { if (!(expr)) failed(#expr, __LINE__); } while (false)


std::vector<double> render(Engine mode, uint32_t seed, int key=60, int partition=1) {
    Synth s; s.setSampleRate(44100);
    Patch p; p.engine=mode; p.seed=seed; s.setPatch(p);
    std::vector<double> result;
    s.noteOn(key, .9, 7);
    for (int n=0; n < 44100 * 2; n += partition) {
        for (int z=0; z < partition && n+z < 44100*2; ++z) {
            if (n+z == 22050) s.noteOff(key, 7);
            const auto pair = s.renderFrame();
            CHECK(std::isfinite(pair[0]) && std::isfinite(pair[1]));
            CHECK(std::abs(pair[0]) <= 0.98 && std::abs(pair[1]) <= 0.98);
            result.push_back(pair[0]);
        }
    }
    return result;
}

int main() {
    std::array<std::vector<double>,2> bank;
    for(int m=0;m<2;++m) {
        bank[m]=render(static_cast<Engine>(m), 0x125A3030u);
        const auto same=render(static_cast<Engine>(m), 0x125A3030u, 60, 127);
        CHECK(bank[m]==same);  // exact stream-partition independence
        const auto other=render(static_cast<Engine>(m), 0x125A3031u);
        double energy=0, difference=0;
        for(size_t i=0;i<bank[m].size();++i) {
            energy+=bank[m][i]*bank[m][i];
            difference+=std::pow(bank[m][i]-other[i],2);
        }
        CHECK(energy>1.e-9 && difference>1.e-9);
        std::cout << "Engine " << m << " RMS=" << std::sqrt(energy/bank[m].size())
                  << " differentSeedRMS=" << std::sqrt(difference/bank[m].size()) << "\n";
        CHECK(std::sqrt(energy/bank[m].size())>0.0001); // audible signal; no verdict on musical quality
        // Atonal engines do not derive their pitch from MIDI key.
        { const auto otherKey=render(static_cast<Engine>(m), 0x125A3030u, 72);
            // Trigger key does not change an atonal event (when sequence and seed match).
            CHECK(otherKey==bank[m]);
        }
    }
    for(int i=0;i<2;++i) for(int j=i+1;j<2;++j) {
        double diff=0;
        for(size_t k=0;k<bank[i].size();++k) diff+=std::pow(bank[i][k]-bank[j][k],2);
        CHECK(diff>1.e-6);
    }
    // For one-shot metal percussion, NoteOff does not chop the resonant tail.
    const auto beforeRelease=render(Engine::Impact,0x12345u);
    CHECK(!beforeRelease.empty());
    CHECK(nextGeneration(0x125A2026u)!=0x125A2026u);
    // Stress all 12 voices, extreme controls and common host sample rates.
    for(double rate : {44100.0,48000.0,96000.0,192000.0}) {
        Synth crowded; crowded.setSampleRate(rate);
        Patch extreme; extreme.engine=Engine::Impact; extreme.force=1.0;
        extreme.chaos=1.0; extreme.decay=1.0; extreme.level=1.0;
        crowded.setPatch(extreme);
        for(int key=36;key<48;++key)crowded.noteOn(key,1.0,key);
        CHECK(crowded.activeVoices()==Synth::kVoices);
        for(int frame=0;frame<int(rate*0.35);++frame) {
            const auto x=crowded.renderFrame();
            CHECK(std::isfinite(x[0])&&std::isfinite(x[1]));
            CHECK(std::abs(x[0])<=0.98&&std::abs(x[1])<=0.98);
        }
        crowded.allNotesOff();
        crowded.reset();
        CHECK(crowded.activeVoices()==0);
    }
    // Legacy tracking has no effect on atonal percussion, even when old state sets 100%.
    Synth legacy1,legacy2;
    Patch k; k.engine=Engine::Perc; k.keyTrack=1.; k.seed=0x12345u;
    legacy1.setPatch(k); k.keyTrack=0.; legacy2.setPatch(k);
    legacy1.noteOn(48,1);legacy2.noteOn(72,1);
    for(int i=0;i<20000;++i)CHECK(legacy1.renderFrame()==legacy2.renderFrame());
    // Panic clears all voices; ordinary NoteOff preserves percussion tails.
    Synth p; p.noteOn(60,1.); CHECK(p.activeVoices()==1);
    p.noteOff(60); CHECK(p.activeVoices()==1);
    p.allNotesOff(); CHECK(p.activeVoices()==0);
    // V1/V2 legacy state migrations to the two remaining metal-drum engines.
    CHECK(migrateStoredEngine(0)==Engine::Impact);
    CHECK(migrateStoredEngine(1)==Engine::Perc);
    CHECK(migrateStoredEngine(2)==Engine::Perc);
    CHECK(migrateStoredEngine(3)==Engine::Impact);
    std::cout << "Core DSP QA PASS\n";
}
