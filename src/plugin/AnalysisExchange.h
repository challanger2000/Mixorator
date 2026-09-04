#pragma once

#include "../analysis/AssessmentModel.h"
#include "pluginterfaces/vst/ivstdataexchange.h"

#include <cstdint>
#include <type_traits>

namespace Mixorator
{
constexpr Steinberg::Vst::DataExchangeUserContextID kAnalysisExchangeContext = 0x4D58u;

constexpr const char* kSetAnalysisStateMessage = "Mixorator.SetAnalysisState";
constexpr const char* kAnalysisStateKey = "State";
constexpr Steinberg::int64 kAnalysisStateLive = 0;
constexpr Steinberg::int64 kAnalysisStateFinal = 1;

constexpr const char* kRequestFinalSnapshotMessage = "Mixorator.RequestFinalSnapshot";
constexpr const char* kFinalSnapshotMessage = "Mixorator.FinalSnapshot";
constexpr const char* kFinalSnapshotGenerationKey = "Generation";
constexpr const char* kFinalSnapshotDataKey = "Snapshot";

struct AnalysisExchangePacket
{
    std::uint64_t sequence {0};
    std::uint64_t finalizationGeneration {0};
    std::uint8_t finalState {0};
    Analysis::Metrics metrics {};
};

static_assert(std::is_trivially_copyable_v<AnalysisExchangePacket>,
              "Data Exchange packet must remain trivially copyable");
}
