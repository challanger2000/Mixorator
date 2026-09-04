#include "AssessmentModel.h"

#include <algorithm>
#include <cmath>

namespace Mixorator::Analysis
{
namespace
{
struct Profile
{
    double loudnessMin;
    double loudnessMax;
    double plrMin;
    double plrMax;
    double lraMin;
    double lraMax;
};

Profile profileFor(AnalysisMode mode, Genre genre, Era era) noexcept
{
    const bool vintage = era == Era::Vintage;

    if (mode == AnalysisMode::Mix)
    {
        switch (genre)
        {
            case Genre::Metal: return {-24.0, -14.0, 8.0, 18.0, 2.0, 14.0};
            case Genre::Pop: return {-24.0, -14.0, 8.0, 18.0, 2.0, 14.0};
            case Genre::Techno:
            case Genre::HouseEdm: return {-22.0, -13.0, 7.0, 17.0, 2.0, 12.0};
            case Genre::HipHopTrap: return {-23.0, -13.0, 7.0, 18.0, 2.0, 13.0};
            case Genre::Classical: return {-30.0, -16.0, 12.0, 28.0, 5.0, 24.0};
            case Genre::Jazz:
            case Genre::AcousticFolk: return {-28.0, -15.0, 10.0, 24.0, 4.0, 20.0};
            default: return {-26.0, -14.0, 9.0, 21.0, 3.0, 18.0};
        }
    }

    switch (genre)
    {
        case Genre::Metal: return vintage ? Profile{-16.0, -8.0, 8.0, 18.0, 2.0, 14.0} : Profile{-12.0, -6.0, 5.0, 13.0, 1.0, 10.0};
        case Genre::Pop: return vintage ? Profile{-17.0, -9.0, 8.0, 18.0, 2.0, 14.0} : Profile{-14.0, -7.0, 6.0, 14.0, 1.0, 11.0};
        case Genre::Techno:
        case Genre::HouseEdm: return vintage ? Profile{-16.0, -9.0, 7.0, 17.0, 2.0, 12.0} : Profile{-12.0, -6.0, 5.0, 12.0, 1.0, 9.0};
        case Genre::HipHopTrap: return vintage ? Profile{-17.0, -9.0, 8.0, 18.0, 2.0, 13.0} : Profile{-13.0, -6.0, 5.0, 13.0, 1.0, 10.0};
        case Genre::Classical: return {-24.0, -14.0, 14.0, 30.0, 6.0, 25.0};
        case Genre::Jazz: return vintage ? Profile{-22.0, -13.0, 12.0, 26.0, 5.0, 22.0} : Profile{-20.0, -11.0, 10.0, 23.0, 4.0, 20.0};
        case Genre::AcousticFolk: return vintage ? Profile{-22.0, -13.0, 11.0, 25.0, 4.0, 21.0} : Profile{-19.0, -10.0, 9.0, 21.0, 3.0, 18.0};
        case Genre::Cinematic: return {-22.0, -10.0, 10.0, 26.0, 4.0, 22.0};
        default: return vintage ? Profile{-20.0, -11.0, 10.0, 23.0, 4.0, 19.0} : Profile{-16.0, -8.0, 7.0, 18.0, 2.0, 15.0};
    }
}

double rangeScore(double value, double minGood, double maxGood, double softMargin) noexcept
{
    if (!std::isfinite(value))
        return 0.0;
    if (value >= minGood && value <= maxGood)
        return 100.0;
    const double distance = value < minGood ? minGood - value : value - maxGood;
    return std::clamp(100.0 - (distance / softMargin) * 50.0, 0.0, 100.0);
}

Verdict verdictFor(double score) noexcept
{
    if (score >= 90.0) return Verdict::Excellent;
    if (score >= 75.0) return Verdict::Good;
    if (score >= 50.0) return Verdict::Attention;
    return Verdict::Critical;
}
}

Assessment AssessmentModel::evaluate(const Metrics& m, AnalysisMode mode, Genre genre, Era era) noexcept
{
    Assessment a;

    if (!std::isfinite(m.integratedLufs) || !std::isfinite(m.truePeakDbtp))
    {
        a.technicalVerdict = a.styleVerdict = a.deliveryVerdict = a.overallVerdict = Verdict::InsufficientData;
        a.technicalScore = a.styleScore = a.deliveryScore = a.overallScore = 0.0;
        return a;
    }

    // Technical safety is deliberately genre-independent.
    double technical = 100.0;
    if (m.nonFiniteSamples > 0) technical = 0.0;
    if (m.clippedSamples > 0) technical -= 25.0;
    if (m.truePeakDbtp > 0.0) technical -= std::min(35.0, 15.0 + m.truePeakDbtp * 10.0);
    if (m.correlation < 0.0) technical -= std::min(30.0, -m.correlation * 30.0);
    if (m.monoCompatibilityDb < -3.0) technical -= std::min(25.0, (-3.0 - m.monoCompatibilityDb) * 5.0);
    if (std::abs(m.lrBalanceDb) > 3.0) technical -= std::min(15.0, (std::abs(m.lrBalanceDb) - 3.0) * 3.0);
    if (m.dcOffsetLeftDbfs > -50.0 || m.dcOffsetRightDbfs > -50.0) technical -= 10.0;
    technical = std::clamp(technical, 0.0, 100.0);

    const Profile p = profileFor(mode, genre, era);
    const double loudnessScore = rangeScore(m.integratedLufs, p.loudnessMin, p.loudnessMax, 6.0);
    const double plrScore = rangeScore(m.plrDb, p.plrMin, p.plrMax, 6.0);
    const double lraScore = rangeScore(m.lraLu, p.lraMin, p.lraMax, 8.0);
    double style = 0.45 * loudnessScore + 0.35 * plrScore + 0.20 * lraScore;

    // Tonal profile remains a weak signal until empirical genre corridors are calibrated.
    const double tonalTotal = m.tonalPercent[0] + m.tonalPercent[1] + m.tonalPercent[2] + m.tonalPercent[3];
    if (tonalTotal > 99.0 && tonalTotal < 101.0)
    {
        const bool extreme = m.tonalPercent[0] > 65.0 || m.tonalPercent[3] > 45.0;
        if (extreme) style = std::max(0.0, style - 5.0);
    }

    // Delivery compatibility is intentionally separate from musical master quality.
    double delivery = 100.0;
    if (m.truePeakDbtp > -1.0)
        delivery -= std::min(35.0, (m.truePeakDbtp + 1.0) * 20.0 + 10.0);
    if (m.integratedLufs > -14.0)
        delivery -= std::min(20.0, (m.integratedLufs + 14.0) * 2.5);
    delivery = std::clamp(delivery, 0.0, 100.0);

    // Critical technical defects cap the overall result; style cannot average them away.
    double overall = 0.50 * technical + 0.35 * style + 0.15 * delivery;
    if (technical < 50.0) overall = std::min(overall, 49.0);
    else if (technical < 75.0) overall = std::min(overall, 74.0);

    a.technicalScore = technical;
    a.styleScore = std::clamp(style, 0.0, 100.0);
    a.deliveryScore = delivery;
    a.overallScore = std::clamp(overall, 0.0, 100.0);
    a.technicalVerdict = verdictFor(a.technicalScore);
    a.styleVerdict = verdictFor(a.styleScore);
    a.deliveryVerdict = verdictFor(a.deliveryScore);
    a.overallVerdict = verdictFor(a.overallScore);

    return a;
}
}
