#include "public.sdk/source/main/pluginfactory.h"
#include "MixoratorProcessor.h"

#define stringPluginName "Mixorator"

BEGIN_FACTORY_DEF("challanger2000", "https://github.com/challanger2000/Mixorator", "")

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Steinberg::FUID(0x4D69786F, 0x7261746F, 0x72000000, 0x00000001)),
    Steinberg::PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Steinberg::Vst::kDistributable,
    "Fx|Analyzer",
    FULL_VERSION_STR,
    kVstVersionString,
    Mixorator::Processor::createInstance)

END_FACTORY
