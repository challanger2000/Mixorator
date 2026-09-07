#include "analysis/AssessmentModel.h"

#include <cmath>
#include <iostream>

namespace
{
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

bool approx(double a, double b, double tolerance = 1e-12)
{
    return std::abs(a - b) <= tolerance;
}

Mixorator::Analysis::Metrics baseMetrics()
{
    using namespace Mixorator::Analysis;
    Metrics m;
    m.integratedLufs = -10.0;
    m.truePeakDbtp = -1.2;
    m.plrDb = 10.0;
    m.lraLu = 5.0;
    m.correlation = 0.8;
    m.monoCompatibilityDb = -0.5;
    m.worstLocalCorrelation = 0.8;
    m.worstLocalMonoCompatibilityDb = -0.5;
    m.negativeCorrelationPercent = 0.0;
    m.lrBalanceDb = 0.0;
    m.dcOffsetLeftDbfs = -90.0;
    m.dcOffsetRightDbfs = -90.0;
    return m;
}

Mixorator::Analysis::Metrics tonal(std::array<double, 8> values)
{
    auto m = baseMetrics();
    m.detailedTonalPercent = values;
    return m;
}
}

int main()
{
    using namespace Mixorator::Analysis;

    {
        Metrics m = baseMetrics();
        if (AssessmentModel::detailedTonalDataAvailable(m))
            return fail("Empty detailed tonal data was treated as valid");
        if (AssessmentModel::detailedTonalScore(m, Genre::General) != 0.0)
            return fail("Unavailable detailed tonal data did not remain neutral to calibration scoring");
    }

    {
        Metrics m = tonal({{18.0, 24.0, 10.0, 22.0, 12.0, 7.0, 4.0, 3.0}});
        if (!AssessmentModel::detailedTonalDataAvailable(m))
            return fail("Normalized detailed tonal data was rejected");
        const double score = AssessmentModel::detailedTonalScore(m, Genre::HouseEdm);
        if (!std::isfinite(score) || score < 0.0 || score > 100.0)
            return fail("Detailed tonal calibration score left valid range");
    }

    // First measured musical anchors. These are not target curves: they protect
    // the calibration layer from rejecting known-good but radically different
    // spectral balances merely because their raw energy percentages differ.
    const Metrics glow = tonal({{41.4372,36.8340,9.2006,9.9428,1.3891,.6383,.4488,.1093}});
    const Metrics ricky = tonal({{.6874,25.6551,21.2383,41.9215,9.4837,.7246,.2507,.0386}});
    const Metrics winnetou = tonal({{.4809,15.2061,32.3142,46.0280,5.7695,.1649,.0310,.0053}});
    const Metrics tchaikovsky = tonal({{2.2699,18.2910,27.6430,42.7908,8.7523,.1840,.0396,.0293}});
    const Metrics israel = tonal({{.0342,20.5488,70.7207,7.2947,.7576,.3366,.2462,.0611}});

    for (const Metrics* reference : {&glow, &ricky, &winnetou, &tchaikovsky, &israel})
    {
        if (!AssessmentModel::detailedTonalDataAvailable(*reference))
            return fail("Measured musical tonal reference was rejected as invalid");
    }

    // Broad seed profiles should at least distinguish unmistakable spectral
    // tendencies. Keep these synthetic tests because they test directionality,
    // not conformity to one commercial master.
    {
        Metrics edm = tonal({{20.0, 24.0, 8.0, 20.0, 12.0, 7.0, 5.0, 4.0}});
        const double edmAsEdm = AssessmentModel::detailedTonalScore(edm, Genre::HouseEdm);
        const double edmAsAcoustic = AssessmentModel::detailedTonalScore(edm, Genre::AcousticFolk);
        if (edmAsEdm <= edmAsAcoustic + 5.0)
            return fail("Bass-forward electronic balance was not preferred by EDM calibration profile");
    }

    {
        Metrics acoustic = tonal({{2.0, 10.0, 18.0, 42.0, 14.0, 7.0, 4.0, 3.0}});
        const double acousticAsAcoustic = AssessmentModel::detailedTonalScore(acoustic, Genre::AcousticFolk);
        const double acousticAsEdm = AssessmentModel::detailedTonalScore(acoustic, Genre::HouseEdm);
        if (acousticAsAcoustic <= acousticAsEdm + 5.0)
            return fail("Mid-forward acoustic balance was not preferred by Acoustic/Folk calibration profile");
    }

    // The measured anchors demonstrate why raw per-band percentages must remain
    // broad/contextual: Glow is legitimately low-heavy, while Israel is an
    // extreme low-mid-body recording. Require only genre-relative plausibility.
    if (AssessmentModel::detailedTonalScore(glow, Genre::HouseEdm) <=
        AssessmentModel::detailedTonalScore(glow, Genre::AcousticFolk))
        return fail("Measured EDM reference was not preferred by EDM calibration");

    if (AssessmentModel::detailedTonalScore(israel, Genre::AcousticFolk) <=
        AssessmentModel::detailedTonalScore(israel, Genre::HouseEdm))
        return fail("Measured acoustic reference was not preferred by Acoustic/Folk calibration");

    // Related mid-forward orchestral/cinematic anchors should remain plausible
    // without requiring them to share one exact curve.
    if (AssessmentModel::detailedTonalScore(winnetou, Genre::Cinematic) < 70.0)
        return fail("Measured vintage cinematic reference scored implausibly low");
    if (AssessmentModel::detailedTonalScore(tchaikovsky, Genre::Classical) < 70.0)
        return fail("Measured classical reference scored implausibly low");
    if (AssessmentModel::detailedTonalScore(ricky, Genre::Pop) < 70.0)
        return fail("Measured vintage pop reference scored implausibly low");

    // Most important safety invariant: the experimental eight-band calibration
    // surface must not affect production verdicts until explicitly integrated.
    {
        Metrics plain = baseMetrics();
        plain.tonalPercent = {{35.0, 35.0, 25.0, 5.0}};
        Metrics detailed = plain;
        detailed.detailedTonalPercent = {{20.0, 24.0, 8.0, 20.0, 12.0, 7.0, 5.0, 4.0}};

        const auto before = AssessmentModel::evaluate(plain, AnalysisMode::Master, Genre::HouseEdm, Era::Modern);
        const auto after = AssessmentModel::evaluate(detailed, AnalysisMode::Master, Genre::HouseEdm, Era::Modern);

        if (!approx(before.technicalScore, after.technicalScore) ||
            !approx(before.styleScore, after.styleScore) ||
            !approx(before.pcmDeliveryScore, after.pcmDeliveryScore) ||
            !approx(before.streamingDeliveryScore, after.streamingDeliveryScore) ||
            !approx(before.overallScore, after.overallScore) ||
            before.technicalVerdict != after.technicalVerdict ||
            before.styleVerdict != after.styleVerdict ||
            before.pcmDeliveryVerdict != after.pcmDeliveryVerdict ||
            before.streamingDeliveryVerdict != after.streamingDeliveryVerdict ||
            before.overallVerdict != after.overallVerdict)
            return fail("Experimental detailed tonal calibration leaked into production verdicts");
    }

    std::cout << "All detailed tonal calibration tests passed.\n";
    return 0;
}
