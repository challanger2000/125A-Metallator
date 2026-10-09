#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
namespace Steinberg::Vst::Metallator {
class Controller final:public EditController {
public:
    static FUnknown* createInstance(void*){return static_cast<IEditController*>(new Controller());}
    tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
    tresult PLUGIN_API setComponentState(IBStream*) SMTG_OVERRIDE;
};
}
