#include "plugin/ControllerStateCodec.h"

#include "public.sdk/source/common/memorystream.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

bool sameMetrics(const Mixorator::Analysis::Metrics& a, const Mixorator::Analysis::Metrics& b)
{
    return a.integratedLufs == b.integratedLufs &&
           a.truePeakDbtp == b.truePeakDbtp &&
           a.plrDb == b.plrDb &&
           a.lraLu == b.lraLu &&
           a.crestFactorDb == b.crestFactorDb &&
           a.correlation == b.correlation &&
           a.monoCompatibilityDb == b.monoCompatibilityDb &&
           a.worstLocalCorrelation == b.worstLocalCorrelation &&
           a.worstLocalMonoCompatibilityDb == b.worstLocalMonoCompatibilityDb &&
           a.negativeCorrelationPercent == b.negativeCorrelationPercent &&
           a.lrBalanceDb == b.lrBalanceDb &&
           a.dcOffsetLeftDbfs == b.dcOffsetLeftDbfs &&
           a.dcOffsetRightDbfs == b.dcOffsetRightDbfs &&
           a.clippedSamples == b.clippedSamples &&
           a.nonFiniteSamples == b.nonFiniteSamples &&
           a.tonalPercent == b.tonalPercent &&
           a.loudnessAvailable == b.loudnessAvailable &&
           a.plrAvailable == b.plrAvailable &&
           a.lraAvailable == b.lraAvailable &&
           a.provisional == b.provisional;
}

bool sameState(const Mixorator::ControllerStateData& a, const Mixorator::ControllerStateData& b)
{
    return a.mode == b.mode && a.genre == b.genre && a.era == b.era &&
           a.language == b.language && a.detailsVisible == b.detailsVisible &&
           a.hasFinal == b.hasFinal && sameMetrics(a.metrics, b.metrics);
}

Mixorator::ControllerStateData makeFinalState()
{
    Mixorator::ControllerStateData state;
    state.mode = Mixorator::Analysis::AnalysisMode::Master;
    state.genre = Mixorator::Analysis::Genre::Cinematic;
    state.era = Mixorator::Analysis::Era::Vintage;
    state.language = Mixorator::Localization::Language::English;
    state.detailsVisible = true;
    state.hasFinal = true;
    auto& m = state.metrics;
    m.integratedLufs = -12.345;
    m.truePeakDbtp = -0.678;
    m.plrDb = 11.25;
    m.lraLu = 4.75;
    m.crestFactorDb = 8.5;
    m.correlation = 0.42;
    m.monoCompatibilityDb = -1.25;
    m.worstLocalCorrelation = -0.17;
    m.worstLocalMonoCompatibilityDb = -4.5;
    m.negativeCorrelationPercent = 12.5;
    m.lrBalanceDb = 0.65;
    m.dcOffsetLeftDbfs = -83.0;
    m.dcOffsetRightDbfs = -79.5;
    m.clippedSamples = 1234567890123ULL;
    m.nonFiniteSamples = 987654321ULL;
    m.tonalPercent = {{17.0, 28.0, 31.0, 24.0}};
    m.loudnessAvailable = true;
    m.plrAvailable = false;
    m.lraAvailable = true;
    m.provisional = false;
    return state;
}

std::vector<char> encode(const Mixorator::ControllerStateData& state)
{
    Steinberg::MemoryStream stream;
    if (!Mixorator::writeControllerState(&stream, state))
        return {};
    return std::vector<char>(stream.getData(), stream.getData() + stream.getSize());
}

bool decode(std::vector<char>& bytes, Mixorator::ControllerStateData& state)
{
    Steinberg::MemoryStream stream(bytes.data(), static_cast<Steinberg::TSize>(bytes.size()));
    return Mixorator::readControllerState(&stream, state);
}

void putInt32Le(std::vector<char>& bytes, std::size_t offset, std::uint32_t value)
{
    bytes[offset + 0] = static_cast<char>(value & 0xFFu);
    bytes[offset + 1] = static_cast<char>((value >> 8) & 0xFFu);
    bytes[offset + 2] = static_cast<char>((value >> 16) & 0xFFu);
    bytes[offset + 3] = static_cast<char>((value >> 24) & 0xFFu);
}
}

int main()
{
    using namespace Mixorator;

    // Full FINAL state must round-trip bit-for-bit through the versioned codec.
    const auto original = makeFinalState();
    auto bytes = encode(original);
    if (bytes.empty())
        return fail("Could not encode FINAL controller state");
    ControllerStateData restored;
    if (!decode(bytes, restored))
        return fail("Could not decode valid FINAL controller state");
    if (!sameState(original, restored))
        return fail("FINAL controller state round-trip changed data");

    // A non-final project state stores only UI choices; no stale result may appear.
    ControllerStateData uiOnly;
    uiOnly.mode = Analysis::AnalysisMode::Master;
    uiOnly.genre = Analysis::Genre::Metal;
    uiOnly.era = Analysis::Era::Vintage;
    uiOnly.language = Localization::Language::English;
    uiOnly.detailsVisible = true;
    uiOnly.hasFinal = false;
    uiOnly.metrics.integratedLufs = -3.0; // intentionally must not be serialized
    auto uiBytes = encode(uiOnly);
    if (uiBytes.empty())
        return fail("Could not encode UI-only controller state");
    ControllerStateData uiRestored;
    if (!decode(uiBytes, uiRestored))
        return fail("Could not decode UI-only controller state");
    if (uiRestored.mode != uiOnly.mode || uiRestored.genre != uiOnly.genre ||
        uiRestored.era != uiOnly.era || uiRestored.language != uiOnly.language ||
        uiRestored.detailsVisible != uiOnly.detailsVisible || uiRestored.hasFinal)
        return fail("UI-only controller state round-trip failed");
    if (uiRestored.metrics.integratedLufs != Analysis::Metrics{}.integratedLufs)
        return fail("UI-only state restored stale analysis metrics");

    // Corrupt input must be rejected transactionally: destination stays unchanged.
    const ControllerStateData sentinel = makeFinalState();
    auto verifyRejectedWithoutMutation = [&](std::vector<char> badBytes, const char* message) -> int {
        ControllerStateData destination = sentinel;
        if (decode(badBytes, destination))
            return fail(message);
        if (!sameState(destination, sentinel))
            return fail("Rejected state partially modified controller data");
        return 0;
    };

    auto badMagic = bytes;
    badMagic[0] ^= 0x5A;
    if (const int result = verifyRejectedWithoutMutation(badMagic, "Invalid state magic was accepted"))
        return result;

    auto futureVersion = bytes;
    putInt32Le(futureVersion, 4, 2u);
    if (const int result = verifyRejectedWithoutMutation(futureVersion, "Unknown state version was accepted"))
        return result;

    auto invalidMode = bytes;
    putInt32Le(invalidMode, 8, 99u);
    if (const int result = verifyRejectedWithoutMutation(invalidMode, "Invalid analysis mode was accepted"))
        return result;

    if (bytes.size() < 8)
        return fail("Encoded state unexpectedly short");
    auto truncated = bytes;
    truncated.resize(truncated.size() - 7);
    if (const int result = verifyRejectedWithoutMutation(truncated, "Truncated state was accepted"))
        return result;

    // Null streams are programming/host errors and must fail cleanly.
    ControllerStateData nullDestination = sentinel;
    if (readControllerState(nullptr, nullDestination) || writeControllerState(nullptr, original))
        return fail("Null stream was accepted");
    if (!sameState(nullDestination, sentinel))
        return fail("Null read modified destination state");

    std::cout << "All controller state codec tests passed.\n";
    return 0;
}
