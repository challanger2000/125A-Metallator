#pragma once
#include "metal_synth.h"
#include "metallatorstate.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
namespace Steinberg::Vst::Metallator {
class Processor final : public AudioEffect {
public:
    Processor();
    static FUnknown* createInstance(void*) { return static_cast<IAudioProcessor*>(new Processor()); }
    tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
    tresult PLUGIN_API setupProcessing(ProcessSetup&) SMTG_OVERRIDE;
    tresult PLUGIN_API setActive(TBool) SMTG_OVERRIDE;
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement*,int32,SpeakerArrangement*,int32) SMTG_OVERRIDE;
    tresult PLUGIN_API canProcessSampleSize(int32) SMTG_OVERRIDE;
    tresult PLUGIN_API process(ProcessData&) SMTG_OVERRIDE;
    tresult PLUGIN_API getState(IBStream*) SMTG_OVERRIDE;
    tresult PLUGIN_API setState(IBStream*) SMTG_OVERRIDE;
private:
    void applyValue(ParamID,ParamValue) noexcept;
    void applyLastChanges(IParameterChanges*) noexcept;
    template<class Sample> void render(ProcessData&) noexcept;
    StateCodec::Values values_ {};
    MetallatorDSP::Synth synth_ {};
};
}
