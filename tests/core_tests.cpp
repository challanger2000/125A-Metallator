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
    std::cout << "Core DSP QA PASS\n";
}
