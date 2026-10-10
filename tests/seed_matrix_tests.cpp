// Frozen seed corpus smoke test for all archetypes and common sample rates.
// Checks quiet-output and channel-balance defects not covered by one favorite seed.
#include "metal_synth.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace MetallatorDSP;
int main(){
    constexpr std::array<uint32_t,8> seeds = {
        307896358u, 0x125A2026u, 0x1234u, 0x12345678u,
        0x7ffdf00du, 0x00410311u, 0x01d376e4u, 0x6e19500cu};
    int cases=0;
    for(double sr:{44100.,48000.,96000.,192000.}) {
        for(unsigned e=0;e<4;++e)for(unsigned shape=0;shape<kArchetypeCount;++shape){
            double minPeak=1,maxPeak=0,maxImbalance=0;
            for(uint32_t seed:seeds){
                Synth s;s.setSampleRate(sr);Patch p;p.engine=static_cast<Engine>(e);
                p.archetype=shape;p.seed=seed;s.setPatch(p);s.noteOn(60,.9,9);
                double l2=0,r2=0,peak=0;
                const int n=int(sr*.65);
                for(int i=0;i<n;++i){
                    const auto y=s.renderFrame();
                    if(!std::isfinite(y[0])||!std::isfinite(y[1])||
                        std::max(std::abs(y[0]),std::abs(y[1]))>.98){
                        std::cerr<<"bad sample engine="<<e<<" shape="<<shape<<" seed="<<seed<<'\n';
                        return 1;
                    }
                    l2+=y[0]*y[0];r2+=y[1]*y[1];
                    peak=std::max({peak,std::abs(y[0]),std::abs(y[1])});
                }
                minPeak=std::min(minPeak,peak);
                maxPeak=std::max(maxPeak,peak);
                const double imbalance=std::abs(10.0*std::log10((l2+1e-30)/(r2+1e-30)));
                maxImbalance=std::max(maxImbalance,imbalance);
                if(peak<.02 || imbalance>3.0){
                    std::cerr<<"audibility/stereo fail engine="<<e<<" archetype="<<shape
                             <<" seed="<<seed<<" peak="<<peak<<" balance="<<imbalance<<"dB\n";
                    return 1;
                }
                ++cases;
            }
            std::cout<<"rate="<<sr<<" engine="<<e<<" archetype="<<shape
                     <<" peakRange="<<minPeak<<".."<<maxPeak
                     <<" maxImbalance="<<maxImbalance<<" dB\n";
        }
    }
    std::cout<<"Seed and rate matrix PASS ("<<cases<<" cases)\n";
}
