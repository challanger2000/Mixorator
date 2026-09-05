#include "analysis/AssessmentModel.h"

#include <cmath>
#include <iostream>

namespace
{
using namespace Mixorator::Analysis;

int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

Metrics base()
{
    Metrics m;
    m.integratedLufs = -12.0;
    m.truePeakDbtp = -2.1;
    m.plrDb = 10.0;
    m.lraLu = 6.0;
    m.crestFactorDb = 10.0;
    m.correlation = 0.75;
    m.monoCompatibilityDb = -0.4;
    m.worstLocalCorrelation = 0.60;
    m.worstLocalMonoCompatibilityDb = -0.8;
    m.negativeCorrelationPercent = 0.0;
    m.lrBalanceDb = 0.1;
    m.dcOffsetLeftDbfs = -90.0;
    m.dcOffsetRightDbfs = -90.0;
    return m;
}

bool safeTechnical(const Assessment& a)
{
    return a.technicalScore >= 90.0 && a.technicalVerdict == Verdict::Excellent;
}
}

int main()
{
    using namespace Mixorator::Analysis;

    {
        auto m = base();
        m.integratedLufs = -8.0;
        m.truePeakDbtp = -2.2;
        m.plrDb = 7.0;
        m.lraLu = 4.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (!safeTechnical(a) || a.styleScore < 85.0 || a.overallVerdict == Verdict::Critical)
            return fail("Clean loud modern Metal master was not treated as plausible");
        if (a.streamingDeliveryScore < 90.0)
            return fail("Safe loud Metal master was incorrectly marked as poor streaming delivery");
    }

    {
        auto m = base();
        m.integratedLufs = -16.0;
        m.truePeakDbtp = -2.5;
        m.plrDb = 18.0;
        m.lraLu = 12.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (!safeTechnical(a))
            return fail("Dynamic Metal scenario contaminated technical safety");
        // This is clearly outside the modern-Metal sweet spot, but not so far
        // outside it that it should cross the deliberately stronger UNUSUAL
        // threshold. ATTENTION is the intended intermediate result here.
        if (a.styleScore >= 75.0 || a.styleVerdict != Verdict::Attention)
            return fail("Dynamic modern Metal scenario did not register as stylistically atypical");
    }

    {
        auto m = base();
        m.integratedLufs = -12.0;
        m.truePeakDbtp = -2.5;
        m.plrDb = 5.0;
        m.lraLu = 1.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Classical, Era::Modern);
        if (!safeTechnical(a))
            return fail("Over-compressed Classical scenario contaminated technical safety");
        // Loudness alone is only slightly outside the broad Classical corridor,
        // while PLR and LRA are clearly too compressed. The weighted result is
        // therefore ATTENTION rather than the stronger UNUSUAL classification.
        if (a.styleScore >= 75.0 || a.styleVerdict != Verdict::Attention)
            return fail("Over-compressed Classical scenario was not flagged stylistically");
    }

    {
        auto m = base();
        // Keep loudness/dynamics inside both modern Acoustic/Folk and Techno
        // corridors so this scenario isolates tonal discrimination instead of
        // letting the dynamics profile dominate the comparison.
        m.integratedLufs = -12.0;
        m.plrDb = 10.0;
        m.lraLu = 6.0;
        m.tonalPercent = {{62.0, 18.0, 15.0, 5.0}};
        const auto acoustic = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::AcousticFolk, Era::Modern);
        const auto techno = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Techno, Era::Modern);
        if (acoustic.styleScore >= techno.styleScore)
            return fail("Bass-heavy tonal scenario did not distinguish Acoustic/Folk from Techno");
    }

    {
        auto m = base();
        m.correlation = -0.75;
        m.monoCompatibilityDb = -12.0;
        m.worstLocalCorrelation = -1.0;
        m.worstLocalMonoCompatibilityDb = -20.0;
        m.negativeCorrelationPercent = 35.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Pop, Era::Modern);
        if (a.technicalScore >= 75.0 || a.overallScore >= 75.0)
            return fail("Serious phase/mono problem was averaged away");
        if (a.styleScore < 75.0)
            return fail("Serious phase issue incorrectly contaminated style score");
    }

    {
        auto m = base();
        m.clippedSamples = 5000;
        m.truePeakDbtp = 1.2;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Pop, Era::Modern);
        if (a.technicalScore >= 75.0 || a.pcmDeliveryScore >= 75.0 || a.streamingDeliveryScore >= 75.0)
            return fail("Clipping/positive true peak was not treated as a delivery problem");
    }

    {
        auto m = base();
        m.integratedLufs = -8.0;
        m.truePeakDbtp = -0.8;
        m.plrDb = 7.0;
        m.lraLu = 4.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.styleScore < 85.0 || a.pcmDeliveryScore < 90.0)
            return fail("Streaming headroom concern contaminated musical/PCM master quality");
        if (a.streamingDeliveryScore >= 90.0)
            return fail("Insufficient streaming codec headroom was not flagged");
    }

    {
        auto m = base();
        m.integratedLufs = -18.0;
        m.plrDb = 16.0;
        m.lraLu = 10.0;
        const auto modern = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Rock, Era::Modern);
        const auto vintage = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Rock, Era::Vintage);
        if (vintage.styleScore <= modern.styleScore)
            return fail("Vintage Rock scenario did not prefer the vintage profile");
        if (std::abs(vintage.technicalScore - modern.technicalScore) > 1e-12)
            return fail("Era changed technical safety");
    }

    {
        auto m = base();
        m.integratedLufs = -20.0;
        m.plrDb = 15.0;
        m.lraLu = 10.0;
        const auto mix = AssessmentModel::evaluate(m, AnalysisMode::Mix, Genre::Rock, Era::Modern);
        if (mix.pcmDeliveryVerdict != Verdict::InsufficientData || mix.streamingDeliveryVerdict != Verdict::InsufficientData)
            return fail("MIX scenario received finished-master delivery verdicts");
    }

    std::cout << "All Mixorator master scenario tests passed.\n";
    return 0;
}
