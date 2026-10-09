#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
namespace Steinberg::Vst::Metallator {
enum ParameterIds : ParamID {
    kEngineId = 1000,
    kSizeId = 1001,
    kForceId = 1002,
    kChaosId = 1003,
    kDecayId = 1004,
    kKeyTrackId = 1005,
    kLevelId = 1006,
    kGenerateId = 1007,
    kVariateId = 1008,
    kBypassId = 1009
};
// Brand-new instrument IDs: never reuse the former Metallator audio-effect class IDs.
static DECLARE_UID(ProcessorUID, 0x214A965C, 0xE9144D03, 0x9ACCC5A9, 0xD74C256E);
static DECLARE_UID(ControllerUID, 0xB23D2781, 0xA7024E3D, 0x869275A3, 0xE8C10273);
}
