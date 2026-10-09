#include "metallatorcontroller.h"
#include "metallatorids.h"
#include "metallatorstate.h"
#include "pluginterfaces/base/ibstream.h"
namespace Steinberg::Vst::Metallator {
tresult PLUGIN_API Controller::initialize(FUnknown* ctx){
    const auto result=EditController::initialize(ctx);
    if(result!=kResultOk)return result;
    auto* engine=new StringListParameter(STR16("ENGINE"),kEngineId);
    engine->appendString(STR16("IMPACT"));engine->appendString(STR16("PERC"));
    engine->appendString(STR16("FRICTION"));engine->appendString(STR16("DRONE"));
    parameters.addParameter(engine);
    parameters.addParameter(STR16("SIZE"),STR16("%"),0,0.75,ParameterInfo::kCanAutomate,kSizeId);
    parameters.addParameter(STR16("FORCE"),STR16("%"),0,0.80,ParameterInfo::kCanAutomate,kForceId);
    parameters.addParameter(STR16("CHAOS"),STR16("%"),0,0.55,ParameterInfo::kCanAutomate,kChaosId);
    parameters.addParameter(STR16("DECAY"),STR16("%"),0,0.55,ParameterInfo::kCanAutomate,kDecayId);
    parameters.addParameter(STR16("DRONE KEY TRACK"),STR16("%"),0,0.0,ParameterInfo::kCanAutomate,kKeyTrackId);
    parameters.addParameter(STR16("LEVEL"),STR16("%"),0,0.68,ParameterInfo::kCanAutomate,kLevelId);
    // Generic host editor: toggling either switch regenerates. Native pushbuttons follow in the GUI stage.
    parameters.addParameter(STR16("GENERATE (toggle)"),nullptr,1,0,ParameterInfo::kCanAutomate,kGenerateId);
    parameters.addParameter(STR16("VARIATE (toggle)"),nullptr,1,0,ParameterInfo::kCanAutomate,kVariateId);
    parameters.addParameter(STR16("Bypass"),nullptr,1,0,ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass,kBypassId);
    return kResultOk;
}
tresult PLUGIN_API Controller::setComponentState(IBStream* state){
    StateCodec::Values v;
    if(!StateCodec::read(state,v))return kResultFalse;
    setParamNormalized(kEngineId,static_cast<int>(v.patch.engine)/3.0);
    setParamNormalized(kSizeId,v.patch.size);
    setParamNormalized(kForceId,v.patch.force);
    setParamNormalized(kChaosId,v.patch.chaos);
    setParamNormalized(kDecayId,v.patch.decay);
    setParamNormalized(kKeyTrackId,v.patch.keyTrack);
    setParamNormalized(kLevelId,v.patch.level);
    setParamNormalized(kGenerateId,v.generate);
    setParamNormalized(kVariateId,v.variate);
    setParamNormalized(kBypassId,v.bypass?1.:0.);
    return kResultOk;
}
}
