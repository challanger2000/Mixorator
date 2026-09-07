#include "MixoratorController.h"

#include "base/source/fstreamer.h"

#include <cstdint>

namespace Mixorator
{
namespace
{
constexpr Steinberg::uint32 kStateMagic = 0x414E4C59u; // "ANLY"
constexpr Steinberg::uint32 kStateVersion = 1u;

bool writeMetrics(Steinberg::IBStreamer& streamer, const Analysis::Metrics& m) noexcept
{
    return streamer.writeDouble(m.integratedLufs) &&
           streamer.writeDouble(m.truePeakDbtp) &&
           streamer.writeDouble(m.plrDb) &&
           streamer.writeDouble(m.lraLu) &&
           streamer.writeDouble(m.crestFactorDb) &&
           streamer.writeDouble(m.correlation) &&
           streamer.writeDouble(m.monoCompatibilityDb) &&
           streamer.writeDouble(m.worstLocalCorrelation) &&
           streamer.writeDouble(m.worstLocalMonoCompatibilityDb) &&
           streamer.writeDouble(m.negativeCorrelationPercent) &&
           streamer.writeDouble(m.lrBalanceDb) &&
           streamer.writeDouble(m.dcOffsetLeftDbfs) &&
           streamer.writeDouble(m.dcOffsetRightDbfs) &&
           streamer.writeInt64u(m.clippedSamples) &&
           streamer.writeInt64u(m.nonFiniteSamples) &&
           streamer.writeDouble(m.tonalPercent[0]) &&
           streamer.writeDouble(m.tonalPercent[1]) &&
           streamer.writeDouble(m.tonalPercent[2]) &&
           streamer.writeDouble(m.tonalPercent[3]) &&
           streamer.writeBool(m.loudnessAvailable) &&
           streamer.writeBool(m.plrAvailable) &&
           streamer.writeBool(m.lraAvailable) &&
           streamer.writeBool(m.provisional);
}

bool readMetrics(Steinberg::IBStreamer& streamer, Analysis::Metrics& m) noexcept
{
    return streamer.readDouble(m.integratedLufs) &&
           streamer.readDouble(m.truePeakDbtp) &&
           streamer.readDouble(m.plrDb) &&
           streamer.readDouble(m.lraLu) &&
           streamer.readDouble(m.crestFactorDb) &&
           streamer.readDouble(m.correlation) &&
           streamer.readDouble(m.monoCompatibilityDb) &&
           streamer.readDouble(m.worstLocalCorrelation) &&
           streamer.readDouble(m.worstLocalMonoCompatibilityDb) &&
           streamer.readDouble(m.negativeCorrelationPercent) &&
           streamer.readDouble(m.lrBalanceDb) &&
           streamer.readDouble(m.dcOffsetLeftDbfs) &&
           streamer.readDouble(m.dcOffsetRightDbfs) &&
           streamer.readInt64u(m.clippedSamples) &&
           streamer.readInt64u(m.nonFiniteSamples) &&
           streamer.readDouble(m.tonalPercent[0]) &&
           streamer.readDouble(m.tonalPercent[1]) &&
           streamer.readDouble(m.tonalPercent[2]) &&
           streamer.readDouble(m.tonalPercent[3]) &&
           streamer.readBool(m.loudnessAvailable) &&
           streamer.readBool(m.plrAvailable) &&
           streamer.readBool(m.lraAvailable) &&
           streamer.readBool(m.provisional);
}
}

Steinberg::tresult PLUGIN_API Controller::getState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    Steinberg::IBStreamer streamer(state, Steinberg::kLittleEndian);
    const bool saveFinal = hasDefinitiveFinalSnapshot();

    if (!streamer.writeInt32u(kStateMagic) ||
        !streamer.writeInt32u(kStateVersion) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(uiMode_)) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(uiGenre_)) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(uiEra_)) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(uiLanguage_)) ||
        !streamer.writeBool(uiDetailsVisible_) ||
        !streamer.writeBool(saveFinal))
        return Steinberg::kResultFalse;

    if (saveFinal && !writeMetrics(streamer, latestPacket_.metrics))
        return Steinberg::kResultFalse;

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Controller::setState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    Steinberg::IBStreamer streamer(state, Steinberg::kLittleEndian);
    Steinberg::uint32 magic = 0;
    Steinberg::uint32 version = 0;
    Steinberg::int32 mode = 0;
    Steinberg::int32 genre = 0;
    Steinberg::int32 era = 0;
    Steinberg::int32 language = 0;
    bool detailsVisible = false;
    bool hasFinal = false;
    Analysis::Metrics restoredMetrics {};

    if (!streamer.readInt32u(magic) || !streamer.readInt32u(version) ||
        magic != kStateMagic || version != kStateVersion ||
        !streamer.readInt32(mode) || !streamer.readInt32(genre) ||
        !streamer.readInt32(era) || !streamer.readInt32(language) ||
        !streamer.readBool(detailsVisible) || !streamer.readBool(hasFinal))
        return Steinberg::kResultFalse;

    if (mode < 0 || mode > static_cast<Steinberg::int32>(Analysis::AnalysisMode::Master) ||
        genre < 0 || genre > static_cast<Steinberg::int32>(Analysis::Genre::General) ||
        era < 0 || era > static_cast<Steinberg::int32>(Analysis::Era::Vintage) ||
        language < 0 || language > static_cast<Steinberg::int32>(Localization::Language::English))
        return Steinberg::kResultFalse;

    if (hasFinal && !readMetrics(streamer, restoredMetrics))
        return Steinberg::kResultFalse;

    // Apply only after the complete stream has been validated. A truncated or
    // corrupt project state therefore cannot leave the controller half-restored.
    uiMode_ = static_cast<Analysis::AnalysisMode>(mode);
    uiGenre_ = static_cast<Analysis::Genre>(genre);
    uiEra_ = static_cast<Analysis::Era>(era);
    uiLanguage_ = static_cast<Localization::Language>(language);
    uiDetailsVisible_ = detailsVisible;
    uiHelpVisible_ = false;
    uiAnalysisActive_ = false; // A half-finished measurement is never resumed after reload.
    uiFinalSelected_ = hasFinal;
    requestedFinalGeneration_ = 0;
    acceptFirstPacketAfterQueueOpen_ = true;

    if (hasFinal)
    {
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
