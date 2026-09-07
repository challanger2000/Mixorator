#pragma once

#include "../analysis/AssessmentModel.h"
#include "Localization.h"
#include "pluginterfaces/base/ibstream.h"

namespace Mixorator
{
struct ControllerStateData
{
    Analysis::AnalysisMode mode {Analysis::AnalysisMode::Mix};
    Analysis::Genre genre {Analysis::Genre::General};
    Analysis::Era era {Analysis::Era::Modern};
    Localization::Language language {Localization::Language::German};
    bool detailsVisible {false};
    bool hasFinal {false};
    Analysis::Metrics metrics {};
};

bool writeControllerState(Steinberg::IBStream* stream, const ControllerStateData& data) noexcept;
bool readControllerState(Steinberg::IBStream* stream, ControllerStateData& data) noexcept;
}
