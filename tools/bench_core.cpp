#include "metal_synth.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <cmath>
using namespace MetallatorDSP;
using Clock=std::chrono::steady_clock;
int main(){
    constexpr int block=64,blocks=2800;
    for(unsigned eng=0;eng<2;++eng)for(unsigned shape=0;shape<4;++shape){
        Synth s;s.setSampleRate(48000);
        Patch p;p.engine=static_cast<Engine>(eng);p.archetype=shape;p.seed=0x125A2026u;
        s.setPatch(p);
        for(int i=0;i<12;++i)s.noteOn(36+i,.9,i);
        std::vector<double> ms;ms.reserve(blocks);
        double checksum=0.;
        for(int n=0;n<blocks;++n){
            auto t0=Clock::now();
            for(int j=0;j<block;++j){auto v=s.renderFrame();checksum+=v[0];}
            const auto end=Clock::now();
            ms.push_back(std::chrono::duration<double,std::milli>(end-t0).count());
        }
        std::sort(ms.begin(),ms.end());
        auto q=[&](double p){return ms[std::min(blocks-1,int((blocks-1)*p))];};
        int misses=0;for(auto x:ms)if(x>64./48000.*1000.)++misses;
        std::cout<<"eng="<<eng<<" shape="<<shape<<" p95="<<q(.95)
                 <<"ms p99="<<q(.99)<<"ms max="<<ms.back()<<"ms deadlines="<<misses
                 <<" checksum="<<checksum<<'\n';
    }
}
