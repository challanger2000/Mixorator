#pragma once

#include "AssessmentModel.h"

namespace Mixorator::Analysis
{
enum class DiagnosticEvidence : std::uint8_t
{
    None,
    TonalBalanceExtreme
};

inline DiagnosticEvidence primaryScoreNeutralDiagnosticEvidence(const Metrics& metrics,
                                                                const Assessment& assessment) noexcept
{
    // Keep this layer deliberately conservative. It may surface additional
    // evidence to the user, but must never alter scores or verdicts.
    if (assessment.tonalRatioAnomaly && AssessmentModel::detailedTonalDataAvailable(metrics))
        return DiagnosticEvidence::TonalBalanceExtreme;

    return DiagnosticEvidence::None;
}
}
