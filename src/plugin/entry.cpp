#include "public.sdk/source/main/pluginfactory.h"
#include "AnalysatorController.h"
#include "AnalysatorIDs.h"
#include "AnalysatorProcessor.h"

#define stringPluginName "Analysator"
#define stringPluginVersion "0.1.0"

BEGIN_FACTORY_DEF("challanger2000", "https://github.com/challanger2000/Analysator", "")

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Analysator::kProcessorUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Steinberg::Vst::kDistributable,
    "Fx|Analyzer",
    stringPluginVersion,
    kVstVersionString,
    Analysator::Processor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Analysator::kControllerUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    "Analysator Controller",
    0,
    "",
    stringPluginVersion,
    kVstVersionString,
    Analysator::Controller::createInstance)

END_FACTORY
