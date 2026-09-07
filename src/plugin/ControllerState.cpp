#include "MixoratorController.h"

#include "base/source/fstreamer.h"

#include <cstdint>

namespace Mixorator
{
namespace
{
constexpr Steinberg::uint32 kStateMagic = 0x414E4C59u; // "ANLY"
constexpr Steinberg::uint32 kStateVersion = 1u;

bool writeMetrics(Steinberg::IBStreamer& s, const Analysis::Metrics& m) noexcept
{
    return s.writeDouble(m.integratedLufs) &&
           s.writeDouble(m.truePeakDbtp) &&
           s.writeDouble(m.plrDb) &&
           s.writeDouble(m.lraLu) &&
           s.writeDouble(m.crestFactorDb) &&
           s.writeDouble(m.correlation) &&
           s.writeDouble(m.monoCompatibilityDb) &&
           s.writeDouble(m.worstLocalCorrelation) &&
           s.writeDouble(m.worstLocalMonoCompatibilityDb) &&
           s.writeDouble(m.negativeCorrelationPercent) &&
           s.writeDouble(m.lrBalanceDb) &&
           s.writeDouble(m.dcOffsetLeftDbfs) &&
           s.writeDouble(m.dcOffsetRightDbfs) &&
           s.writeInt64u(m.clippedSamples) &&
           s.writeInt64u(m.nonFiniteSamples) &&
           s.writeDouble(m.tonalPercent[0]) &&
           s.writeDouble(m.tonalPercent[1]) &&
           s.writeDouble(m.tonalPercent[2]) &&
           s.writeDouble(m.tonalPercent[3]) &&
           s.writeBool(m.loudnessAvailable) &&
           s.writeBool(m.plrAvailable) &&
           s.writeBool(m.lraAvailable) &&
           s.writeBool(m.provisional);
}

bool readMetrics(Steinberg::IBStreamer& s, Analysis::Metrics& m) noexcept
{
    return s.readDouble(m.integratedLufs) &&
           s.readDouble(m.truePeakDbtp) &&
           s.readDouble(m.plrDb) &&
           s.readDouble(m.lraLu) &&
           s.readDouble(m.crestFactorDb) &&
           s.readDouble(m.correlation) &&
           s.readDouble(m.monoCompatibilityDb) &&
           s.readDouble(m.worstLocalCorrelation) &&
           s.readDouble(m.worstLocalMonoCompatibilityDb) &&
           s.readDouble(m.negativeCorrelationPercent) &&
           s.readDouble(m.lrBalanceDb) &&
           s.readDouble(m.dcOffsetLeftDbfs) &&
           s.readDouble(m.dcOffsetRightDbfs) &&
           s.readInt64u(m.clippedSamples) &&
           s.readInt64u(m.nonFiniteSamples) &&
           s.readDouble(m.tonalPercent[0]) &&
           s.readDouble(m.tonalPercent[1]) &&
           s.readDouble(m.tonalPercent[2]) &&
           s.readDouble(m.tonalPercent[3]) &&
           s.readBool(m.loudnessAvailable) &&
           s.readBool(m.plrAvailable) &&
           s.readBool(m.lraAvailable) &&
           s.readBool(m.provisional);
}
}

Steinberg::tresult PLUGIN_API Controller::getState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    Steinberg::IBStreamer s(state, Steinberg::kLittleEndian);
    const bool saveFinal = hasDefinitiveFinalSnapshot();

    if (!s.writeInt32u(kStateMagic) ||
        !s.writeInt32u(kStateVersion) ||
        !s.writeInt32(static_cast<Steinberg::int32>(uiMode_)) ||
        !s.writeInt32(static_cast<Steinberg::int32>(uiGenre_)) ||
        !s.writeInt32(static_cast<Steinberg::int32>(uiEra_)) ||
        !s.writeInt32(static_cast<Steinberg::int32>(uiLanguage_)) ||
        !s.writeBool(uiDetailsVisible_) ||
        !s.writeBool(saveFinal))
        return Steinberg::kResultFalse;

    if (saveFinal && !writeMetrics(s, latestPacket_.metrics))
        return Steinberg::kResultFalse;

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Controller::setState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    Steinberg::IBStreamer s(state, Steinberg::kLittleEndian);
    Steinberg::uint32 magic = 0;
    Steinberg::uint32 version = 0;
    Steinberg::int32 mode = 0;
    Steinberg::int32 genre = 0;
    Steinberg::int32 era = 0;
    Steinberg::int32 language = 0;
    bool detailsVisible = false;
    bool hasFinal = false;
    Analysis::Metrics restoredMetrics {};

    if (!s.readInt32u(magic) || !s.readInt32u(version) ||
        magic != kStateMagic || version != kStateVersion ||
        !s.readInt32(mode) || !s.readInt32(genre) ||
        !s.readInt32(era) || !s.readInt32(language) ||
        !s.readBool(detailsVisible) || !s.readBool(hasFinal))
        return Steinberg::kResultFalse;

    if (mode < 0 || mode > static_cast<Steinberg::int32>(Analysis::AnalysisMode::Master) ||
        genre < 0 || genre > static_cast<Steinberg::int32>(Analysis::Genre::General) ||
        era < 0 || era > static_cast<Steinberg::int32>(Analysis::Era::Vintage) ||
        language < 0 || language > static_cast<Steinberg::int32>(Localization::Language::English))
        return Steinberg::kResultFalse;

    if (hasFinal && !readMetrics(s, restoredMetrics))
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
