#pragma once

#include "../analysis/AssessmentModel.h"

#include <cstdint>
#include <type_traits>

namespace Mixorator
{
constexpr Steinberg::Vst::DataExchangeUserContextID kAnalysisExchangeContext = 0x4D58u;

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
