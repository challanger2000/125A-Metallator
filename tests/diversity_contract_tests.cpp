// Contract declared for structural generation. One archetype is NOT a new pitch.
// These 2-second 10ms RMS envelopes are compared after scale normalization, so
// varying a sound's gain or resampling its frequency alone cannot satisfy it.
// This is one morphology gate; it is NOT a complete listening/quality verdict.
#include "metal_synth.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace MetallatorDSP;
[[noreturn]] static void failed(const char* what,double a=0,double b=0) {
    std::cerr << "DIVERSITY CONTRACT FAILED: " << what << " " << a << " " << b << '\n';
    std::exit(1);
}
static constexpr int kFrames=88200,kBins=200,kStep=kFrames/kBins;
struct Summary {
    std::array<double,kBins> env{};
    double peak{0.},rms{0.};
};
static Summary render(Engine engine,unsigned archetype,uint32_t seed,int key=60) {
    Synth synth;
    synth.setSampleRate(44100);
    Patch p;
    p.engine=engine; p.archetype=archetype; p.seed=seed;
    synth.setPatch(p);
    synth.noteOn(key,.9,17);
    Summary out{};double energy=0.;
    for(int i=0;i<kFrames;++i) {
        if(i==57330)synth.noteOff(key,17);
        const auto lr=synth.renderFrame();
        if(!std::isfinite(lr[0])||!std::isfinite(lr[1]) ||
           std::abs(lr[0])>0.98 || std::abs(lr[1])>0.98)
            failed("nonfinite/unsafe output",double(archetype),double(i));
        const double x=.5*(lr[0]+lr[1]);
        out.peak=std::max(out.peak,std::abs(x));
        energy+=x*x;
        out.env[static_cast<size_t>(i/kStep)] += x*x;
    }
    out.rms=std::sqrt(energy/kFrames);
    for(auto& bin:out.env)bin=std::sqrt(bin/kStep);
    double n=0;for(double x:out.env)n+=x*x;
    n=std::sqrt(n);
    if(n<=1e-7||out.peak<=1e-4)failed("silent generated profile",double(archetype),out.peak);
    for(auto& bin:out.env)bin/=n;
    return out;
}
static double similarity(const Summary& a,const Summary& b) {
    double dot=0;for(int i=0;i<kBins;++i)dot+=a.env[size_t(i)]*b.env[size_t(i)];
    return dot;
}
int main(){
    for(int e=0;e<2;++e){
        const auto engine=static_cast<Engine>(e);
        std::array<Summary,kArchetypeCount> outputs{};
        for(unsigned profile=0;profile<kArchetypeCount;++profile) {
            outputs[profile]=render(engine,profile,307896358u);
            std::cout << "engine=" << e << " archetype=" << profile
                      << " peak=" << outputs[profile].peak
                      << " rms=" << outputs[profile].rms << '\n';
            // Atonal drum triggers never track keyboard pitch.
            const auto otherKey=render(engine,profile,307896358u,75);
            if(outputs[profile].env!=otherKey.env||
               outputs[profile].rms!=otherKey.rms)
                failed("key changed atonal event",double(e),double(profile));
            // VARIATE changes fine detail but does not erase identity of its event type.
            const auto varied=render(engine,profile,30583931u);
            const double preserved=similarity(outputs[profile],varied);
            if(preserved<0.55)failed("VARIATE destroyed archetype morphology",profile,preserved);
        }
        for(unsigned a=0;a<kArchetypeCount;++a)
            for(unsigned b=a+1;b<kArchetypeCount;++b){
                const double same=similarity(outputs[a],outputs[b]);
                std::cout<<"engine="<<e<<" GENERATED "<<a<<" vs "<<b
                         <<" time-envelope cosine="<<same<<'\n';
                // Declared acceptance: at least 4 morphologies, all six
                // pairwise scale-invariant comparisons distinguishable.
                // 0.96 is a deliberately conservative contract boundary.
                if(same>=.96)failed("GENERATE changed only pitch/timbre/gain",double(e),same);
            }
    }
    Patch p; p.archetype=0;p.seed=45237;
    const auto seed=p.seed;
    for(unsigned j=1;j<4;++j){generateNewStructure(p);
       if(p.archetype!=j||p.seed==seed)failed("GENERATE no new construction");
       const auto archetype=p.archetype,oldSeed=p.seed;
       variateStructure(p);
       if(p.archetype!=archetype||p.seed==oldSeed)failed("VARIATE changed construction");
    }
    generateNewStructure(p);if(p.archetype!=0)failed("GENERATE cycle missing");
    std::cout<<"Structural GENERATE/VARIATE contract PASS\n";
}
