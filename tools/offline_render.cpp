// Offline reference generator / waveform export. Shares EXACT DSP with VST3; no DAW required.
// Usage: metallator_render <impact|perc|friction|drone> <output.wav> [seed] [archetype:0-3]
#include "metal_synth.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void le16(std::ofstream& out, uint16_t v) {
    out.put(char(v & 255)); out.put(char((v >> 8) & 255));
}
static void le32(std::ofstream& out, uint32_t v) {
    for(unsigned i=0;i<4;++i)out.put(char((v>>(8*i))&255));
}
int main(int argc, char** argv) {
    if(argc < 3) { std::cerr << "Usage: metallator_render <impact|perc|friction|drone> <path.wav> [seed]\n"; return 2; }
    const std::string name=argv[1];
    MetallatorDSP::Engine mode;
    if(name=="impact") mode=MetallatorDSP::Engine::Impact;
    else if(name=="perc") mode=MetallatorDSP::Engine::Perc;
    else if(name=="friction") mode=MetallatorDSP::Engine::Friction;
    else if(name=="drone") mode=MetallatorDSP::Engine::Drone;
    else { std::cerr << "Unknown engine\n"; return 2; }
    uint32_t seed=0x125A2026u;
    if(argc>=4) {
        try { seed=static_cast<uint32_t>(std::stoul(argv[3])); }
        catch(...) { std::cerr << "Invalid seed\n"; return 2; }
    }
    MetallatorDSP::Synth synth;
    synth.setSampleRate(44100);
    MetallatorDSP::Patch patch;
    patch.engine=mode;
    patch.seed=seed;
    if(argc>=5){
        try {const auto profile=std::stoul(argv[4]);
            if(profile>=MetallatorDSP::kArchetypeCount)throw std::out_of_range("archetype");
            patch.archetype=static_cast<uint32_t>(profile);
        }catch(...) {std::cerr<<"Invalid archetype (0-3)\n";return 2;}
    }
    synth.setPatch(patch);
    synth.noteOn(60,1.0,1);
    // Render through complete bounded tail; sustained types released after 1.3 seconds.
    std::vector<int32_t> samples;
    samples.reserve(44100*12);
    double peak=0.0;
    const int maxFrames=44100*32;
    for(int frame=0;frame<maxFrames;++frame) {
        if(frame==57330 && (mode==MetallatorDSP::Engine::Drone || mode==MetallatorDSP::Engine::Friction)) synth.noteOff(60,1);
        auto s=synth.renderFrame();
        for(double x:s){
            if(!std::isfinite(x)) {std::cerr << "Invalid sample\n"; return 1;}
            peak=std::max(peak,std::abs(x));
            samples.push_back(static_cast<int32_t>(std::lrint(std::clamp(x,-1.,1.)*8388607.0)));
        }
        if(frame>1000 && synth.activeVoices()==0) break;
    }
    const uint64_t bytes64=uint64_t(samples.size())*3;
    if(bytes64>0xffffffffu-44u){std::cerr<<"Wave file too large\n";return 1;}
    const uint32_t dataBytes=static_cast<uint32_t>(bytes64);
    std::ofstream out(argv[2], std::ios::binary);
    if(!out) {std::cerr<<"Cannot create WAV\n";return 1;}
    out.write("RIFF",4);le32(out,36+dataBytes);out.write("WAVEfmt ",8);
    le32(out,16);le16(out,1);le16(out,2);le32(out,44100);
    le32(out,44100*6);le16(out,6);le16(out,24);
    out.write("data",4);le32(out,dataBytes);
    for(const auto s:samples){
        const uint32_t w=static_cast<uint32_t>(s);
        out.put(char(w&255));out.put(char((w>>8)&255));out.put(char((w>>16)&255));
    }
    if(!out){std::cerr<<"WAV write failed\n";return 1;}
    std::cout<<name<<" frames="<<(samples.size()/2)<<" peak="<<peak<<" seed="<<seed<<" archetype="<<patch.archetype<<"\n";
}
