#include "analysis/AssessmentDiagnosisText.h"
#include <cstring>
#include <iostream>

namespace
{
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
    using namespace Mixorator::Analysis;

    const auto noneDe = diagnosticText(DiagnosticEvidence::None, DiagnosticLanguage::German);
    const auto noneEn = diagnosticText(DiagnosticEvidence::None, DiagnosticLanguage::English);
    if (noneDe.line1 || noneDe.line2 || noneEn.line1 || noneEn.line2)
        return fail("None evidence produced visible diagnosis text");

    const auto de = diagnosticText(DiagnosticEvidence::TonalBalanceExtreme, DiagnosticLanguage::German);
    const auto en = diagnosticText(DiagnosticEvidence::TonalBalanceExtreme, DiagnosticLanguage::English);

    if (!de.line1 || !de.line2 || !en.line1 || !en.line2)
        return fail("Tonal anomaly diagnosis text is incomplete");

    if (std::strstr(de.line1, "extreme tonale Balance") == nullptr)
        return fail("German diagnosis lost neutral tonal-balance wording");
    if (std::strstr(de.line2, "Hinweis") == nullptr || std::strstr(de.line2, "nicht automatisch korrigieren") == nullptr)
        return fail("German diagnosis no longer communicates evidence-only semantics");

    if (std::strstr(en.line1, "extreme tonal balance") == nullptr)
        return fail("English diagnosis lost neutral tonal-balance wording");
    if (std::strstr(en.line2, "evidence") == nullptr || std::strstr(en.line2, "do not correct automatically") == nullptr)
        return fail("English diagnosis no longer communicates evidence-only semantics");

    std::cout << "All assessment diagnosis tests passed.\n";
    return 0;
}
