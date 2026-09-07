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
        Metrics m = baseMetrics();
        m.detailedTonalPercent = {{18.0, 24.0, 10.0, 22.0, 12.0, 7.0, 4.0, 3.0}};
        if (!AssessmentModel::detailedTonalDataAvailable(m))
            return fail("Normalized detailed tonal data was rejected");
        const double score = AssessmentModel::detailedTonalScore(m, Genre::HouseEdm);
        if (!std::isfinite(score) || score < 0.0 || score > 100.0)
            return fail("Detailed tonal calibration score left valid range");
    }

    // Broad seed profiles should at least distinguish unmistakable spectral
    // tendencies. These tests intentionally avoid narrow target curves.
    {
        Metrics edm = baseMetrics();
        edm.detailedTonalPercent = {{20.0, 24.0, 8.0, 20.0, 12.0, 7.0, 5.0, 4.0}};
        const double edmAsEdm = AssessmentModel::detailedTonalScore(edm, Genre::HouseEdm);
        const double edmAsAcoustic = AssessmentModel::detailedTonalScore(edm, Genre::AcousticFolk);
        if (edmAsEdm <= edmAsAcoustic + 5.0)
            return fail("Bass-forward electronic balance was not preferred by EDM calibration profile");
    }

    {
        Metrics acoustic = baseMetrics();
        acoustic.detailedTonalPercent = {{2.0, 10.0, 18.0, 42.0, 14.0, 7.0, 4.0, 3.0}};
        const double acousticAsAcoustic = AssessmentModel::detailedTonalScore(acoustic, Genre::AcousticFolk);
        const double acousticAsEdm = AssessmentModel::detailedTonalScore(acoustic, Genre::HouseEdm);
        if (acousticAsAcoustic <= acousticAsEdm + 5.0)
            return fail("Mid-forward acoustic balance was not preferred by Acoustic/Folk calibration profile");
    }

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
