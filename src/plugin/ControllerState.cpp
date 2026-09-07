#include "MixoratorController.h"
#include "ControllerStateCodec.h"

namespace Mixorator
{
Steinberg::tresult PLUGIN_API Controller::getState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    ControllerStateData data;
    data.mode = uiMode_;
    data.genre = uiGenre_;
    data.era = uiEra_;
    data.language = uiLanguage_;
    data.detailsVisible = uiDetailsVisible_;
    data.hasFinal = hasDefinitiveFinalSnapshot();
    if (data.hasFinal)
        data.metrics = latestPacket_.metrics;

    return writeControllerState(state, data) ? Steinberg::kResultOk : Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Controller::setState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    ControllerStateData data;
    if (!readControllerState(state, data))
        return Steinberg::kResultFalse;

    // The codec validates the complete stream before returning success, so a
    // truncated/corrupt project state can never leave the controller half-restored.
    uiMode_ = data.mode;
    uiGenre_ = data.genre;
    uiEra_ = data.era;
    uiLanguage_ = data.language;
    uiDetailsVisible_ = data.detailsVisible;
    uiHelpVisible_ = false;
    uiAnalysisActive_ = false; // A half-finished measurement is never resumed after reload.
    uiFinalSelected_ = data.hasFinal;
    requestedFinalGeneration_ = 0;
    acceptFirstPacketAfterQueueOpen_ = true;

    if (data.hasFinal)
    {
        auto restoredMetrics = data.metrics;
        restoredMetrics.provisional = false;
        latestPacket_ = {};
        latestPacket_.sequence = 1;
        latestPacket_.finalizationGeneration = 1;
        latestPacket_.finalState = 1;
        latestPacket_.metrics = restoredMetrics;
        hasPacket_ = true;
        finalSnapshotGeneration_ = 1;
    }
    else
    {
        latestPacket_ = {};
        hasPacket_ = false;
        finalSnapshotGeneration_ = 0;
    }

    refreshUi();
    return Steinberg::kResultOk;
}
}
