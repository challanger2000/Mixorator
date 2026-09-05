#include "public.sdk/source/main/pluginfactory.h"
#include "MixoratorController.h"
#include "MixoratorIDs.h"
#include "MixoratorProcessor.h"

#define stringPluginName "Analysator"
#define stringPluginVersion "0.1.0"

BEGIN_FACTORY_DEF("challanger2000", "https://github.com/challanger2000/Mixorator", "")

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Mixorator::kProcessorUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Steinberg::Vst::kDistributable,
    "Fx|Analyzer",
    stringPluginVersion,
    kVstVersionString,
    Mixorator::Processor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Mixorator::kControllerUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    "Analysator Controller",
    0,
    "",
    stringPluginVersion,
    kVstVersionString,
    Mixorator::Controller::createInstance)

END_FACTORY
