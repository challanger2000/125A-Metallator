#include "metallatorprocessor.h"
#include "metallatorcontroller.h"
#include "metallatorids.h"
#include "version.h"
#include "public.sdk/source/main/pluginfactory_constexpr.h"
#define stringPluginName "125A Metallator"
BEGIN_FACTORY_DEF(stringCompanyName,stringCompanyWeb,stringCompanyEmail,2)
DEF_CLASS(Steinberg::Vst::Metallator::ProcessorUID,
          Steinberg::PClassInfo::kManyInstances,
          kVstAudioEffectClass,
          stringPluginName,
          Steinberg::Vst::kDistributable,
          "Instrument|Synth",
          FULL_VERSION_STR,
          kVstVersionString,
          Steinberg::Vst::Metallator::Processor::createInstance,nullptr)
DEF_CLASS(Steinberg::Vst::Metallator::ControllerUID,
          Steinberg::PClassInfo::kManyInstances,
          kVstComponentControllerClass,
          stringPluginName " Controller",
          0,"",
          FULL_VERSION_STR,
          kVstVersionString,
          Steinberg::Vst::Metallator::Controller::createInstance,nullptr)
END_FACTORY
