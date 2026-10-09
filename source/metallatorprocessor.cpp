#include "metallatorprocessor.h"
#include "metallatorids.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace Steinberg::Vst::Metallator {
namespace {
constexpr int kNumParams=10;
bool recognized(ParamID id) noexcept {return id>=kEngineId && id<=kBypassId;}
}
Processor::Processor() {setControllerClass(ControllerUID);synth_.setPatch(values_.patch);}
tresult PLUGIN_API Processor::initialize(FUnknown* context) {
    const auto result=AudioEffect::initialize(context);
    if(result!=kResultOk)return result;
    addAudioOutput(STR16("Stereo Output"),SpeakerArr::kStereo);
    addEventInput(STR16("MIDI Input"),1);
    return kResultOk;
}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
    const auto result=AudioEffect::setupProcessing(setup);
    if(result!=kResultOk)return result;
    synth_.setSampleRate(setup.sampleRate);
    synth_.setPatch(values_.patch);
    return kResultOk;
}
tresult PLUGIN_API Processor::setActive(TBool active) {
    if(active)synth_.reset();
    return AudioEffect::setActive(active);
}
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* inputs,int32 nIn,
                                                   SpeakerArrangement* outputs,int32 nOut) {
    if(nIn!=0||nOut!=1||!outputs||
       (outputs[0]!=SpeakerArr::kStereo&&outputs[0]!=SpeakerArr::kMono))return kResultFalse;
    return AudioEffect::setBusArrangements(inputs,nIn,outputs,nOut);
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 size) {
    return (size==kSample32||size==kSample64)?kResultTrue:kResultFalse;
}
void Processor::applyValue(ParamID id,ParamValue val) noexcept {
    if(!std::isfinite(val))return;
    const double x=MetallatorDSP::unit(val);
    switch(id) {
        case kEngineId: values_.patch.engine=static_cast<MetallatorDSP::Engine>(std::clamp(int(std::lround(x*3.0)),0,3)); break;
        case kSizeId: values_.patch.size=x; break;
        case kForceId: values_.patch.force=x; break;
        case kChaosId: values_.patch.chaos=x; break;
        case kDecayId: values_.patch.decay=x; break;
        case kKeyTrackId: values_.patch.keyTrack=x; break;
        case kLevelId: values_.patch.level=x; break;
        case kGenerateId:
            if((x>=0.5)!=(values_.generate>=0.5)){
                // Toggle from either position: every click produces a new repeatable sound.
                values_.patch.seed=MetallatorDSP::nextGeneration(values_.patch.seed)&0x7fffffffu;
                if(values_.patch.seed==0)values_.patch.seed=1;
            }
            values_.generate=x;
            break;
        case kVariateId:
            if((x>=0.5)!=(values_.variate>=0.5)) {
                values_.patch.seed=MetallatorDSP::nextGeneration(values_.patch.seed^0xA11CE5u)&0x7fffffffu;
                if(values_.patch.seed==0)values_.patch.seed=1;
            }
            values_.variate=x;
            break;
        case kBypassId:if((x>=0.5)!=values_.bypass)synth_.reset();values_.bypass=x>=0.5;break;
        default:return;
    }
    synth_.setPatch(values_.patch);
}
void Processor::applyLastChanges(IParameterChanges* changes) noexcept {
    if(!changes)return;
    const int32 count=changes->getParameterCount();
    for(int32 i=0;i<count;++i){
        auto* q=changes->getParameterData(i);
        if(!q||!recognized(q->getParameterId())||q->getPointCount()<1)continue;
        int32 offset=0;ParamValue val=0;
        if(q->getPoint(q->getPointCount()-1,offset,val)==kResultTrue)applyValue(q->getParameterId(),val);
    }
}
template<class Sample> void Processor::render(ProcessData& data) noexcept {
    struct Cursor {IParamValueQueue* q {nullptr};int32 pos{0},count{0},offset{0};ParamValue value{0};bool valid{false};};
    std::array<Cursor,kNumParams> cursors{};
    int used=0;
    if(auto* changes=data.inputParameterChanges){
        const int n=changes->getParameterCount();
        for(int i=0;i<n;++i){
            auto* q=changes->getParameterData(i);
            if(!q||!recognized(q->getParameterId())||q->getPointCount()<=0||used==kNumParams)continue;
            auto& c=cursors[used++];c.q=q;c.count=q->getPointCount();
            c.valid=q->getPoint(0,c.offset,c.value)==kResultTrue;
        }
    }
    const int channels=std::min<int>(2,data.outputs[0].numChannels);
    std::array<Sample*,2> outputs{};
    for(int ch=0;ch<channels;++ch){
        if constexpr(sizeof(Sample)==sizeof(Sample32))outputs[ch]=reinterpret_cast<Sample*>(data.outputs[0].channelBuffers32[ch]);
        else outputs[ch]=reinterpret_cast<Sample*>(data.outputs[0].channelBuffers64[ch]);
    }
    int32 eventIndex=0;
    const int32 eventCount=data.inputEvents?data.inputEvents->getEventCount():0;
    Event nextEvent{};
    bool hasEvent=data.inputEvents&&eventCount>0&&data.inputEvents->getEvent(0,nextEvent)==kResultOk;
    bool anyAudio=false;
    for(int32 i=0;i<data.numSamples;++i) {
        for(int c=0;c<used;++c){
            auto& q=cursors[c];
            while(q.valid&&q.offset<=i){
                applyValue(q.q->getParameterId(),q.value);
                ++q.pos;
                q.valid=q.pos<q.count&&q.q->getPoint(q.pos,q.offset,q.value)==kResultTrue;
            }
        }
        while(hasEvent&&nextEvent.sampleOffset<=i){
            if(nextEvent.type==Event::kNoteOnEvent){
                if(nextEvent.noteOn.velocity>0) synth_.noteOn(nextEvent.noteOn.pitch,nextEvent.noteOn.velocity,nextEvent.noteOn.noteId);
                else synth_.noteOff(nextEvent.noteOn.pitch,nextEvent.noteOn.noteId);
            }else if(nextEvent.type==Event::kNoteOffEvent){
                synth_.noteOff(nextEvent.noteOff.pitch,nextEvent.noteOff.noteId);
            }
            ++eventIndex;
            hasEvent=eventIndex<eventCount&&data.inputEvents->getEvent(eventIndex,nextEvent)==kResultOk;
        }
        const auto voiceActive=synth_.activeVoices()>0;
        const auto frame=values_.bypass?std::array<double,2>{0.0,0.0}:synth_.renderFrame();
        anyAudio |= (voiceActive&&!values_.bypass);
        for(int ch=0;ch<channels;++ch){if(outputs[ch])outputs[ch][i]=static_cast<Sample>(frame[ch]);}
    }
    data.outputs[0].silenceFlags=anyAudio?0:((1ULL<<channels)-1ULL);
}
tresult PLUGIN_API Processor::process(ProcessData& data){
    if(data.numSamples<=0){applyLastChanges(data.inputParameterChanges);return kResultOk;}
    if(data.numOutputs<1||!data.outputs||data.outputs[0].numChannels<1)return kResultFalse;
    if(data.symbolicSampleSize==kSample32)render<Sample32>(data);
    else if(data.symbolicSampleSize==kSample64)render<Sample64>(data);
    else return kResultFalse;
    return kResultOk;
}
tresult PLUGIN_API Processor::getState(IBStream* state){return StateCodec::write(state,values_)?kResultOk:kResultFalse;}
tresult PLUGIN_API Processor::setState(IBStream* state){
    StateCodec::Values newValues;
    if(!StateCodec::read(state,newValues))return kResultFalse;
    values_=newValues;synth_.setPatch(values_.patch);synth_.reset();return kResultOk;
}
} // namespace Steinberg::Vst::Metallator
