#include "analysis/AssessmentModel.h"

#include <iostream>

namespace
{
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

Mixorator::Analysis::Metrics cleanReference()
{
    Mixorator::Analysis::Metrics m;
    m.correlation = 0.75;
    m.monoCompatibilityDb = -0.5;
    m.worstLocalCorrelation = 0.75;
    m.worstLocalMonoCompatibilityDb = -0.5;
    m.negativeCorrelationPercent = 0.0;
    m.lrBalanceDb = 0.0;
    m.dcOffsetLeftDbfs = -90.0;
    m.dcOffsetRightDbfs = -90.0;
    m.clippedSamples = 0;
    m.nonFiniteSamples = 0;
    return m;
}
}

int main()
{
    using namespace Mixorator::Analysis;

    // Reference A: "Menschlichkeit". Loud and controlled, but not an
    // extreme loudness/PLR combination. Density logic must leave technical
    // quality untouched.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -11.4;
        m.truePeakDbtp = -0.5;
        m.plrDb = 10.8;
        m.lraLu = 3.6;
        m.correlation = 0.73;
        m.monoCompatibilityDb = -0.6;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.technicalScore < 99.0)
            return fail("Menschlichkeit-like reference was penalized for healthy master density");
    }

    // Reference B: "Aloha Oe". The master is technically clean at the peak,
    // but -5.9 LUFS together with 5.5 dB PLR is an extreme density trade-off.
    // It may remain an excellent style match, but technical quality must not
    // claim EXCELLENT.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -5.9;
        m.truePeakDbtp = -0.4;
        m.plrDb = 5.5;
        m.lraLu = 3.8;
        m.correlation = 0.81;
        m.monoCompatibilityDb = -0.4;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.technicalVerdict != Verdict::Good || a.technicalScore < 75.0 || a.technicalScore >= 90.0)
            return fail("Aloha-Oe-like extreme master density was not downgraded to GOOD technical quality");
        if (a.styleVerdict != Verdict::Excellent)
            return fail("Aloha-Oe-like intentional density incorrectly damaged Metal style match");
        if (a.pcmDeliveryVerdict != Verdict::Excellent)
            return fail("Aloha-Oe-like density incorrectly contaminated PCM delivery safety");
    }

    // The density trade-off belongs to finished-master assessment only.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -5.9;
        m.truePeakDbtp = -0.4;
        m.plrDb = 5.5;
        m.lraLu = 3.8;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Mix, Genre::Metal, Era::Modern);
        if (a.technicalVerdict != Verdict::Excellent)
            return fail("Finished-master density rule leaked into MIX technical quality");
    }

    // Reference C: "Lost In Shadow". Dynamics are healthy, but +1.7 dBTP is
    // the actual technical fault. The new density rule must not be the reason
    // for the warning.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -9.2;
        m.truePeakDbtp = 1.7;
        m.plrDb = 10.8;
        m.lraLu = 2.4;
        m.correlation = 0.79;
        m.monoCompatibilityDb = -0.5;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Electronic, Era::Modern);
        if (a.technicalVerdict != Verdict::Attention)
            return fail("Lost-In-Shadow-like true-peak fault was not kept distinct from density scoring");
        if (a.pcmDeliveryVerdict != Verdict::Attention)
            return fail("Lost-In-Shadow-like +1.7 dBTP did not lower PCM delivery assessment");
    }

    std::cout << "Real-world reference assessment tests passed.\n";
    return 0;
}
