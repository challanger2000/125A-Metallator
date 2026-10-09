#pragma once
#include "metal_synth.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <cmath>
namespace Steinberg::Vst::Metallator::StateCodec {
static constexpr int32 kVersion=1;
struct Values {
    MetallatorDSP::Patch patch {};
    double generate {0.0};
    double variate {0.0};
    bool bypass {false};
};
inline bool read(IBStream* stream, Values& dest) {
    if(!stream) return false;
    IBStreamer reader(stream, kLittleEndian);
    int32 version=0, mode=0, seed=0, bypass=0;
    float size=0, force=0, chaos=0, decay=0, keyTrack=0, level=0, generate=0, variate=0;
    if(!reader.readInt32(version)||version!=kVersion||!reader.readInt32(mode)||
       !reader.readFloat(size)||!reader.readFloat(force)||!reader.readFloat(chaos)||
       !reader.readFloat(decay)||!reader.readFloat(keyTrack)||!reader.readFloat(level)||
       !reader.readInt32(seed)||!reader.readFloat(generate)||!reader.readFloat(variate)||
       !reader.readInt32(bypass))return false;
    const auto inRange=[](double x){return std::isfinite(x)&&x>=0.0&&x<=1.0;};
    if(mode<0||mode>3||seed<=0||(bypass!=0&&bypass!=1)||
       !inRange(size)||!inRange(force)||!inRange(chaos)||!inRange(decay)||
       !inRange(keyTrack)||!inRange(level)||!inRange(generate)||!inRange(variate))return false;
    Values v;
    v.patch.engine=static_cast<MetallatorDSP::Engine>(mode);
    v.patch.size=size;v.patch.force=force;v.patch.chaos=chaos;
    v.patch.decay=decay;v.patch.keyTrack=keyTrack;v.patch.level=level;
    v.patch.seed=static_cast<uint32_t>(seed);
    v.generate=generate;v.variate=variate;v.bypass=bypass!=0;
    dest=v;
    return true;
}
inline bool write(IBStream* stream,const Values& v) {
    if(!stream) return false;
    IBStreamer writer(stream,kLittleEndian);
    return writer.writeInt32(kVersion)&&
           writer.writeInt32(static_cast<int32>(v.patch.engine))&&
           writer.writeFloat(static_cast<float>(v.patch.size))&&
           writer.writeFloat(static_cast<float>(v.patch.force))&&
           writer.writeFloat(static_cast<float>(v.patch.chaos))&&
           writer.writeFloat(static_cast<float>(v.patch.decay))&&
           writer.writeFloat(static_cast<float>(v.patch.keyTrack))&&
           writer.writeFloat(static_cast<float>(v.patch.level))&&
           writer.writeInt32(static_cast<int32>(v.patch.seed&0x7fffffffu))&&
           writer.writeFloat(static_cast<float>(v.generate))&&
           writer.writeFloat(static_cast<float>(v.variate))&&
           writer.writeInt32(v.bypass?1:0);
}
} // namespace Steinberg::Vst::Metallator::StateCodec
