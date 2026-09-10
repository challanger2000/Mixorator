#include "ControllerStateCodec.h"

#include "base/source/fstreamer.h"

namespace Analysator
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

bool writeControllerState(Steinberg::IBStream* stream, const ControllerStateData& data) noexcept
{
    if (!stream)
        return false;

    Steinberg::IBStreamer streamer(stream, kLittleEndian);
    if (!streamer.writeInt32u(kStateMagic) ||
        !streamer.writeInt32u(kStateVersion) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(data.mode)) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(data.genre)) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(data.era)) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(data.language)) ||
        !streamer.writeBool(data.detailsVisible) ||
        !streamer.writeBool(data.hasFinal))
        return false;

    return !data.hasFinal || writeMetrics(streamer, data.metrics);
}

bool readControllerState(Steinberg::IBStream* stream, ControllerStateData& data) noexcept
{
    if (!stream)
        return false;

    Steinberg::IBStreamer streamer(stream, kLittleEndian);
    Steinberg::uint32 magic = 0;
    Steinberg::uint32 version = 0;
    Steinberg::int32 mode = 0;
    Steinberg::int32 genre = 0;
    Steinberg::int32 era = 0;
    Steinberg::int32 language = 0;
    bool detailsVisible = false;
    bool hasFinal = false;
    Analysis::Metrics metrics {};

    if (!streamer.readInt32u(magic) || !streamer.readInt32u(version) ||
        magic != kStateMagic || version != kStateVersion ||
        !streamer.readInt32(mode) || !streamer.readInt32(genre) ||
        !streamer.readInt32(era) || !streamer.readInt32(language) ||
        !streamer.readBool(detailsVisible) || !streamer.readBool(hasFinal))
        return false;

    if (mode < 0 || mode > static_cast<Steinberg::int32>(Analysis::AnalysisMode::Master) ||
        genre < 0 || genre > static_cast<Steinberg::int32>(Analysis::Genre::General) ||
        era < 0 || era > static_cast<Steinberg::int32>(Analysis::Era::Vintage) ||
        language < 0 || language > static_cast<Steinberg::int32>(Localization::Language::English))
        return false;

    if (hasFinal && !readMetrics(streamer, metrics))
        return false;

    ControllerStateData decoded;
    decoded.mode = static_cast<Analysis::AnalysisMode>(mode);
    decoded.genre = static_cast<Analysis::Genre>(genre);
    decoded.era = static_cast<Analysis::Era>(era);
    decoded.language = static_cast<Localization::Language>(language);
    decoded.detailsVisible = detailsVisible;
    decoded.hasFinal = hasFinal;
    decoded.metrics = metrics;
    data = decoded;
    return true;
}
}
