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

bool finiteRatios(const Mixorator::Analysis::TonalRatioFeatures& f)
{
    return std::isfinite(f.subVsBassDb) && std::isfinite(f.lowMidVsMidDb) &&
           std::isfinite(f.presenceVsMidDb) && std::isfinite(f.upperPresenceVsPresenceDb) &&
           std::isfinite(f.brillianceVsUpperPresenceDb) && std::isfinite(f.airVsBrillianceDb) &&
           std::isfinite(f.lowVsMidDb) && std::isfinite(f.highVsMidDb);
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
        const auto f = AssessmentModel::tonalRatioFeatures(m);
        if (!approx(f.subVsBassDb, 0.0) || !approx(f.highVsMidDb, 0.0))
            return fail("Unavailable tonal data produced non-neutral ratio features");
    }

    {
        Metrics equal = tonal({{12.5,12.5,12.5,12.5,12.5,12.5,12.5,12.5}});
        const auto f = AssessmentModel::tonalRatioFeatures(equal);
        if (!finiteRatios(f))
            return fail("Equal-band tonal ratios were not finite");
        if (!approx(f.subVsBassDb, 0.0) || !approx(f.lowMidVsMidDb, 0.0) ||
            !approx(f.presenceVsMidDb, 0.0) || !approx(f.upperPresenceVsPresenceDb, 0.0) ||
            !approx(f.brillianceVsUpperPresenceDb, 0.0) || !approx(f.airVsBrillianceDb, 0.0) ||
            !approx(f.lowVsMidDb, 0.0) || !approx(f.highVsMidDb, 10.0 * std::log10(2.0)))
            return fail("Logarithmic tonal ratio extraction returned unexpected equal-band values");
    }

    {
        Metrics m = tonal({{18.0,24.0,10.0,22.0,12.0,7.0,4.0,3.0}});
        const double score = AssessmentModel::detailedTonalScore(m, Genre::HouseEdm);
        if (!std::isfinite(score) || score < 0.0 || score > 100.0)
            return fail("Detailed tonal calibration score left valid range");
    }

    const Metrics glow = tonal({{41.4372,36.8340,9.2006,9.9428,1.3891,.6383,.4488,.1093}});
    const Metrics ricky = tonal({{.6874,25.6551,21.2383,41.9215,9.4837,.7246,.2507,.0386}});
    const Metrics winnetou = tonal({{.4809,15.2061,32.3142,46.0280,5.7695,.1649,.0310,.0053}});
    const Metrics tchaikovsky = tonal({{2.2699,18.2910,27.6430,42.7908,8.7523,.1840,.0396,.0293}});
    const Metrics israel = tonal({{.0342,20.5488,70.7207,7.2947,.7576,.3366,.2462,.0611}});

    for (const Metrics* reference : {&glow,&ricky,&winnetou,&tchaikovsky,&israel})
    {
        if (!AssessmentModel::detailedTonalDataAvailable(*reference))
            return fail("Measured musical tonal reference was rejected as invalid");
        if (!finiteRatios(AssessmentModel::tonalRatioFeatures(*reference)))
            return fail("Measured musical reference produced invalid tonal ratios");
    }

    // Ratio features must expose the large-scale contrasts that raw percentages
    // revealed without treating absolute upper-band energy as a universal target.
    const auto glowRatios = AssessmentModel::tonalRatioFeatures(glow);
    const auto israelRatios = AssessmentModel::tonalRatioFeatures(israel);
    if (glowRatios.lowVsMidDb <= 4.0)
        return fail("Measured EDM reference did not expose its strong low-frequency tilt");
    if (israelRatios.lowMidVsMidDb <= 8.0)
        return fail("Measured acoustic reference did not expose its strong low-mid body");
    if (glowRatios.lowMidVsMidDb >= israelRatios.lowMidVsMidDb)
        return fail("Ratio features failed to distinguish EDM from acoustic low-mid structure");

    // Legacy raw-band calibration remains available during migration and still
    // protects the known musical anchors until ratio profiles replace it.
    if (AssessmentModel::detailedTonalScore(glow, Genre::HouseEdm) <=
        AssessmentModel::detailedTonalScore(glow, Genre::AcousticFolk))
        return fail("Measured EDM reference was not preferred by EDM calibration");
    if (AssessmentModel::detailedTonalScore(israel, Genre::AcousticFolk) <=
        AssessmentModel::detailedTonalScore(israel, Genre::HouseEdm))
        return fail("Measured acoustic reference was not preferred by Acoustic/Folk calibration");
    if (AssessmentModel::detailedTonalScore(winnetou, Genre::Cinematic) < 70.0)
        return fail("Measured vintage cinematic reference scored implausibly low");
    if (AssessmentModel::detailedTonalScore(tchaikovsky, Genre::Classical) < 70.0)
        return fail("Measured classical reference scored implausibly low");
    if (AssessmentModel::detailedTonalScore(ricky, Genre::Pop) < 70.0)
        return fail("Measured vintage pop reference scored implausibly low");

    // Most important safety invariant: detailed calibration and the new ratio
    // features still cannot alter production verdicts.
    {
        Metrics plain = baseMetrics();
        plain.tonalPercent = {{35.0,35.0,25.0,5.0}};
        Metrics detailed = plain;
        detailed.detailedTonalPercent = {{20.0,24.0,8.0,20.0,12.0,7.0,5.0,4.0}};
        const auto before = AssessmentModel::evaluate(plain, AnalysisMode::Master, Genre::HouseEdm, Era::Modern);
        const auto after = AssessmentModel::evaluate(detailed, AnalysisMode::Master, Genre::HouseEdm, Era::Modern);
        if (!approx(before.technicalScore,after.technicalScore) || !approx(before.styleScore,after.styleScore) ||
            !approx(before.pcmDeliveryScore,after.pcmDeliveryScore) || !approx(before.streamingDeliveryScore,after.streamingDeliveryScore) ||
            !approx(before.overallScore,after.overallScore) || before.technicalVerdict != after.technicalVerdict ||
            before.styleVerdict != after.styleVerdict || before.pcmDeliveryVerdict != after.pcmDeliveryVerdict ||
            before.streamingDeliveryVerdict != after.streamingDeliveryVerdict || before.overallVerdict != after.overallVerdict)
            return fail("Experimental detailed tonal calibration leaked into production verdicts");
    }

    std::cout << "All detailed tonal calibration tests passed.\n";
    return 0;
}
