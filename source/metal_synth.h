#pragma once
// Two atonal, sample-free metal percussion families: IMPACT and PERC.
// The fixed voice implementation is realtime allocation-free and deterministic.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "impact_body.h"
#include "mechanical_body.h"

namespace MetallatorDSP {
enum class Engine : unsigned { Impact=0, Perc=1 };
// Legacy instrument state V1/V2 had four engines. The two retired
// modes are deliberately migrated, never resurrected in the DSP.
constexpr Engine migrateStoredEngine(unsigned oldMode) noexcept {
    return (oldMode == 1 || oldMode == 2) ? Engine::Perc : Engine::Impact;
}
static constexpr uint32_t kArchetypeCount = 4;
struct Patch {
    Engine engine{Engine::Impact};
    double size{0.75}, force{0.80}, chaos{0.55}, decay{0.55};
    double keyTrack{0.0}; // Legacy state field only; inert and not in the GUI.
    double level{0.68};
    uint32_t seed{0x125A2026u}; // VARIATE: changes microgeometry in current archetype.
    uint32_t archetype{0};       // GENERATE: chooses genuinely different construction.
};
inline double unit(double x) noexcept { return std::isfinite(x) ? std::clamp(x,0.,1.) : 0.; }
inline uint32_t scrambled(uint32_t x) noexcept {
    x ^= x >> 16; x*=0x7feb352du; x^= x>>15;
    x*=0x846ca68bu; x^=x>>16;
    return x ? x : 0x9e3779b9u;
}
inline uint32_t nextGeneration(uint32_t current) noexcept { return scrambled(current+0x9e3779b9u); }
// The two user actions are distinct contracts, used by processor and tests.
inline void generateNewStructure(Patch& p) noexcept {
    p.archetype=(p.archetype+1u)%kArchetypeCount;
    p.seed=nextGeneration(p.seed)&0x7fffffffu;
    if(!p.seed)p.seed=1;
}
inline void variateStructure(Patch& p) noexcept {
    p.seed=nextGeneration(p.seed ^ 0xA11CE5u)&0x7fffffffu;
    if(!p.seed)p.seed=1;
}

class Synth final {
public:
    static constexpr int kVoices=12;
    static constexpr int kModes=14; // legacy display compatibility; not modal engine limit.
    void setSampleRate(double rate) noexcept {
        rate_ = std::isfinite(rate) ? std::clamp(rate,8000.,384000.) : 44100.;
        reset();
    }
    double sampleRate() const noexcept { return rate_; }
    void setPatch(Patch p) noexcept {
        p.size=unit(p.size);p.force=unit(p.force);p.chaos=unit(p.chaos);
        p.decay=unit(p.decay);p.keyTrack=unit(p.keyTrack);p.level=unit(p.level);
        if(static_cast<unsigned>(p.engine)>1)p.engine=Engine::Impact;
        p.archetype %= kArchetypeCount;
        if(!p.seed)p.seed=1;
        patch_=p;
    }
    const Patch& patch() const noexcept { return patch_; }
    void reset() noexcept { voices_={}; voiceIndex_=0; hitIndex_=0; }
    void noteOn(int key,double velocity,int32_t noteId=-1) noexcept {
        if(key<0||key>127||!(velocity>0.0))return;
        Voice& v=voices_[static_cast<std::size_t>(findVoice())];
        v={};v.active=true;v.key=key;v.noteId=noteId;
        v.mode=patch_.engine;v.archetype=patch_.archetype;
        v.velocity=unit(velocity);
        const uint32_t seed=scrambled(patch_.seed ^ (0x9e3779b9u*(++hitIndex_)));
        if(v.mode==Engine::Impact && v.archetype==0) {
            v.impact.strike(rate_,patch_.size,patch_.force,patch_.chaos,patch_.decay,
                            scrambled(seed ^ 0x25a4e913u));
            v.maxAge=v.impact.maxSamples();
        } else {
            // Keyboard note selects the event, never a forced resonator pitch.
            v.body.start(static_cast<unsigned>(v.mode),v.archetype,rate_,
                          patch_.size,patch_.force,patch_.chaos,patch_.decay,seed);
            v.maxAge=v.body.maxSamples();
        }
    }
    void noteOff(int key,int32_t noteId=-1) noexcept {
        for(auto& v:voices_) {
            if(v.active&&v.key==key&&(noteId<0||v.noteId==noteId)) {
                v.released=true; // Percussion tail rings naturally after NoteOff.
            }
        }
    }
    // Panic operation must stop sound; ordinary NoteOff does not chop percussion tails.
    void allNotesOff() noexcept { for(auto& v:voices_)v.active=false; }
    int activeVoices() const noexcept {
        int n=0;for(const auto& v:voices_)n+=int(v.active);return n;
    }
    std::array<double,2> renderFrame() noexcept {
        double l=0,r=0;
        for(auto& v:voices_){
            if(!v.active)continue;
            double vl=0,vr=0;
            if(v.mode==Engine::Impact&&v.archetype==0){
                const auto a=v.impact.render();vl=a.l;vr=a.r;
            }else{
                const auto a=v.body.render();vl=a.l;vr=a.r;
            }
            const double velocity=v.velocity*.75;
            l+=vl*velocity;r+=vr*velocity;
            if(++v.age>=v.maxAge)v.active=false;
        }
        const double gain=.26*patch_.level;
        return {softCeiling(l*gain),softCeiling(r*gain)};
    }
private:
    static double softCeiling(double x) noexcept {
        constexpr double knee=.70, headroom=.28;
        const double a=std::abs(x);
        if(a<=knee)return x;
        const double d=a-knee;
        return std::copysign(knee+headroom*d/(headroom+d),x);
    }
    struct Voice {
        bool active{false},released{false};
        int key{0},noteId{-1};
        Engine mode{Engine::Impact};
        uint32_t archetype{0};
        uint64_t age{0},maxAge{0};
        double velocity{1};
        ImpactBody impact{};
        MechanicalBody body{};
    };
    int findVoice() noexcept {
        for(int i=0;i<kVoices;++i)if(!voices_[i].active)return i;
        const int index=int(voiceIndex_%kVoices);++voiceIndex_;return index;
    }
    Patch patch_{};
    double rate_{44100};
    std::array<Voice,kVoices> voices_{};
    uint32_t hitIndex_{0},voiceIndex_{0};
};
} // namespace MetallatorDSP
