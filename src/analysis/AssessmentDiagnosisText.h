#pragma once

#include "AssessmentDiagnostics.h"

namespace Mixorator::Analysis
{
enum class DiagnosticLanguage : std::uint8_t
{
    German,
    English
};

struct DiagnosticText
{
    const char* line1 {nullptr};
    const char* line2 {nullptr};
};

inline DiagnosticText diagnosticText(DiagnosticEvidence evidence,
                                     DiagnosticLanguage language) noexcept
{
    const bool de = language == DiagnosticLanguage::German;

    switch (evidence)
    {
        case DiagnosticEvidence::TonalBalanceExtreme:
            return de
                ? DiagnosticText{
                    "Ungewöhnlich extreme tonale Balance erkannt.",
                    "Als Hinweis verstehen: Frequenzbalance prüfen, nicht automatisch korrigieren."}
                : DiagnosticText{
                    "Unusually extreme tonal balance detected.",
                    "Treat this as evidence: check spectral balance; do not correct automatically."};

        case DiagnosticEvidence::None:
        default:
            return {};
    }
}
}
