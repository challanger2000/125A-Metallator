#include "metal_synth.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace MetallatorDSP;

std::vector<double> render(Engine mode, uint32_t seed, int key=60, int partition=1) {
    Synth s; s.setSampleRate(44100);
    Patch p; p.engine=mode; p.seed=seed; s.setPatch(p);
    std::vector<double> result;
    s.noteOn(key, .9, 7);
    for (int n=0; n < 44100 * 2; n += partition) {
        for (int z=0; z < partition && n+z < 44100*2; ++z) {
            if (n+z == 22050) s.noteOff(key, 7);
            const auto pair = s.renderFrame();
            assert(std::isfinite(pair[0]) && std::isfinite(pair[1]));
            assert(std::abs(pair[0]) <= 0.98 && std::abs(pair[1]) <= 0.98);
            result.push_back(pair[0]);
        }
    }
    return result;
}

int main() {
    std::array<std::vector<double>,4> bank;
    for(int m=0;m<4;++m) {
        bank[m]=render(static_cast<Engine>(m), 0x125A3030u);
        const auto same=render(static_cast<Engine>(m), 0x125A3030u, 60, 127);
        assert(bank[m]==same);  // exact stream-partition independence
        const auto other=render(static_cast<Engine>(m), 0x125A3031u);
        double energy=0, difference=0;
        for(size_t i=0;i<bank[m].size();++i) {
            energy+=bank[m][i]*bank[m][i];
            difference+=std::pow(bank[m][i]-other[i],2);
        }
        assert(energy>1.e-9 && difference>1.e-9);
        std::cout << "Engine " << m << " RMS=" << std::sqrt(energy/bank[m].size())
                  << " differentSeedRMS=" << std::sqrt(difference/bank[m].size()) << "\n";
        assert(std::sqrt(energy/bank[m].size())>0.0001); // audible signal; no verdict on musical quality
        // Atonal engines do not derive their pitch from MIDI key.
        if(m<3) { const auto otherKey=render(static_cast<Engine>(m), 0x125A3030u, 72);
            // Trigger key does not change an atonal event (when sequence and seed match).
            assert(otherKey==bank[m]);
        }
    }
    for(int i=0;i<4;++i) for(int j=i+1;j<4;++j) {
        double diff=0;
        for(size_t k=0;k<bank[i].size();++k) diff+=std::pow(bank[i][k]-bank[j][k],2);
        assert(diff>1.e-6);
    }
    Synth s; s.setSampleRate(44100); Patch p; p.engine=Engine::Drone; p.keyTrack=1.0;
    s.setPatch(p); s.noteOn(60, 1); for(int i=0;i<100;++i) s.renderFrame();
    s.noteOff(60); for(int i=0;i<44100*6 && s.activeVoices();++i) s.renderFrame();
    assert(s.activeVoices()==0);
    assert(nextGeneration(0x125A2026u)!=0x125A2026u);
    // Stress all 12 voices, extreme controls and common host sample rates.
    for(double rate : {44100.0,48000.0,96000.0,192000.0}) {
        Synth crowded; crowded.setSampleRate(rate);
        Patch extreme; extreme.engine=Engine::Impact; extreme.force=1.0;
        extreme.chaos=1.0; extreme.decay=1.0; extreme.level=1.0;
        crowded.setPatch(extreme);
        for(int key=36;key<48;++key)crowded.noteOn(key,1.0,key);
        assert(crowded.activeVoices()==Synth::kVoices);
        for(int frame=0;frame<int(rate*0.35);++frame) {
            const auto x=crowded.renderFrame();
            assert(std::isfinite(x[0])&&std::isfinite(x[1]));
            assert(std::abs(x[0])<=0.98&&std::abs(x[1])<=0.98);
        }
        crowded.allNotesOff();
        crowded.reset();
        assert(crowded.activeVoices()==0);
    }
    // Key tracking is opt-in and applies only to the pitched DRONE resonances.
    const auto droneA=render(Engine::Drone,0x12345u,48);
    const auto droneB=render(Engine::Drone,0x12345u,72);
    assert(droneA==droneB); // Key tracking off by default.
    Synth keyed1,keyed2; keyed1.setSampleRate(44100);keyed2.setSampleRate(44100);
    Patch keyedPatch; keyedPatch.engine=Engine::Drone; keyedPatch.keyTrack=1.0;
    keyed1.setPatch(keyedPatch);keyed2.setPatch(keyedPatch);
    keyed1.noteOn(48,1);keyed2.noteOn(72,1);
    double tunedDifference=0.0;
    for(int i=0;i<44100;++i) {
        const double delta=keyed1.renderFrame()[0]-keyed2.renderFrame()[0];
        tunedDifference+=delta*delta;
    }
    assert(tunedDifference>1.e-6);
    std::cout << "Core DSP QA PASS\n";
}
