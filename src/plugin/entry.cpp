#include "public.sdk/source/main/pluginfactory.h"
#include "MixoratorController.h"
#include "MixoratorIDs.h"
#include "MixoratorProcessor.h"

#define stringPluginName "Mixorator"

BEGIN_FACTORY_DEF("challanger2000", "https://github.com/challanger2000/Mixorator", "")

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Mixorator::kProcessorUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Steinberg::Vst::kDistributable,
    "Fx|Analyzer",
    FULL_VERSION_STR,
    kVstVersionString,
    Mixorator::Processor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Mixorator::kControllerUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    "Mixorator Controller",
    0,
    "",
    FULL_VERSION_STR,
    kVstVersionString,
    Mixorator::Controller::createInstance)

END_FACTORY
