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
    double loudnessMargin;
    double plrMargin;
    double lraMargin;
    double loudnessWeight;
    double plrWeight;
    double lraWeight;
};

// These are deliberately broad plausibility corridors, not mastering targets.
// They encode expected professional practice by mode/genre/era while avoiding
// fake precision. Distribution-normalisation recommendations are scored later
// and never used as the musical-quality target itself.
Profile profileFor(AnalysisMode mode, Genre genre, Era era) noexcept
{
    const bool vintage = era == Era::Vintage;

    if (mode == AnalysisMode::Mix)
    {
        switch (genre)
        {
            case Genre::Rock:              return {-27.0, -14.0,  9.0, 22.0, 3.0, 18.0, 7.0, 7.0, 9.0, 0.35, 0.40, 0.25};
            case Genre::Metal:             return {-25.0, -13.0,  8.0, 19.0, 2.0, 14.0, 7.0, 6.0, 8.0, 0.35, 0.45, 0.20};
            case Genre::Pop:               return {-25.0, -13.0,  8.0, 19.0, 2.0, 14.0, 7.0, 6.0, 8.0, 0.35, 0.45, 0.20};
            case Genre::Techno:            return {-23.0, -12.0,  7.0, 17.0, 1.5, 12.0, 6.0, 6.0, 7.0, 0.35, 0.45, 0.20};
            case Genre::HouseEdm:          return {-23.0, -12.0,  7.0, 17.0, 1.5, 12.0, 6.0, 6.0, 7.0, 0.35, 0.45, 0.20};
            case Genre::HipHopTrap:        return {-24.0, -12.0,  7.0, 18.0, 1.5, 13.0, 7.0, 6.0, 8.0, 0.35, 0.45, 0.20};
            case Genre::ElectronicAmbient:return {-29.0, -13.0,  8.0, 24.0, 2.0, 20.0, 8.0, 8.0,10.0, 0.30, 0.40, 0.30};
            case Genre::AcousticFolk:      return {-30.0, -15.0, 11.0, 26.0, 4.0, 22.0, 8.0, 8.0,10.0, 0.25, 0.40, 0.35};
            case Genre::Jazz:              return {-30.0, -15.0, 11.0, 27.0, 4.0, 23.0, 8.0, 8.0,10.0, 0.25, 0.40, 0.35};
            case Genre::Classical:         return {-34.0, -16.0, 14.0, 32.0, 6.0, 28.0, 9.0,10.0,12.0, 0.20, 0.40, 0.40};
            case Genre::Cinematic:         return {-31.0, -14.0, 11.0, 29.0, 4.0, 25.0, 9.0, 9.0,11.0, 0.25, 0.40, 0.35};
            case Genre::General:           return {-28.0, -13.0,  9.0, 23.0, 3.0, 19.0, 8.0, 8.0,10.0, 0.30, 0.45, 0.25};
        }
    }

    // MASTER: era changes musical loudness/dynamics expectations, never technical safety.
    switch (genre)
    {
        case Genre::Rock:
            return vintage
                ? Profile{-20.0, -9.0, 10.0, 24.0, 3.0, 19.0, 6.0, 7.0, 9.0, 0.35, 0.40, 0.25}
                : Profile{-15.0, -7.0,  7.0, 18.0, 2.0, 14.0, 5.0, 6.0, 8.0, 0.40, 0.40, 0.20};
        case Genre::Metal:
            return vintage
                ? Profile{-18.0, -8.0,  9.0, 21.0, 2.0, 16.0, 6.0, 7.0, 8.0, 0.40, 0.40, 0.20}
                : Profile{-13.0, -5.5, 5.0, 14.0, 1.0, 10.0, 5.0, 5.0, 7.0, 0.45, 0.40, 0.15};
        case Genre::Pop:
            return vintage
                ? Profile{-19.0, -9.0,  9.0, 21.0, 2.0, 16.0, 6.0, 7.0, 8.0, 0.40, 0.40, 0.20}
                : Profile{-15.0, -6.5, 6.0, 15.0, 1.0, 11.0, 5.0, 5.0, 7.0, 0.45, 0.40, 0.15};
        case Genre::Techno:
        case Genre::HouseEdm:
            return vintage
                ? Profile{-18.0, -8.5, 8.0, 19.0, 2.0, 14.0, 6.0, 6.0, 8.0, 0.45, 0.40, 0.15}
                : Profile{-13.0, -5.5, 4.5, 13.0, 1.0,  9.0, 5.0, 5.0, 6.0, 0.45, 0.40, 0.15};
        case Genre::HipHopTrap:
            return vintage
                ? Profile{-19.0, -9.0,  9.0, 21.0, 2.0, 15.0, 6.0, 7.0, 8.0, 0.40, 0.40, 0.20}
                : Profile{-14.0, -5.5, 5.0, 14.0, 1.0, 10.0, 5.0, 5.0, 7.0, 0.45, 0.40, 0.15};
        case Genre::ElectronicAmbient:
            return vintage
                ? Profile{-23.0, -10.0,10.0, 26.0, 4.0, 22.0, 7.0, 8.0,10.0, 0.30, 0.40, 0.30}
                : Profile{-20.0, -8.0,  8.0, 22.0, 3.0, 19.0, 7.0, 7.0, 9.0, 0.30, 0.40, 0.30};
        case Genre::AcousticFolk:
            return vintage
                ? Profile{-24.0, -12.0,12.0, 28.0, 5.0, 23.0, 7.0, 8.0,10.0, 0.25, 0.40, 0.35}
                : Profile{-21.0, -9.5, 9.0, 23.0, 3.0, 20.0, 7.0, 7.0, 9.0, 0.25, 0.40, 0.35};
        case Genre::Jazz:
            return vintage
                ? Profile{-24.0, -12.0,13.0, 29.0, 5.0, 24.0, 7.0, 8.0,10.0, 0.25, 0.40, 0.35}
                : Profile{-22.0, -10.0,10.0, 25.0, 4.0, 21.0, 7.0, 8.0, 9.0, 0.25, 0.40, 0.35};
        case Genre::Classical:
            return {-27.0, -13.0, 15.0, 34.0, 7.0, 30.0, 9.0,11.0,13.0, 0.15, 0.40, 0.45};
        case Genre::Cinematic:
            return {-24.0, -9.0, 11.0, 29.0, 4.0, 25.0, 8.0, 9.0,11.0, 0.25, 0.40, 0.35};
        case Genre::General:
            return vintage
                ? Profile{-22.0, -10.0,11.0, 26.0, 4.0, 21.0, 7.0, 8.0,10.0, 0.30, 0.45, 0.25}
                : Profile{-18.0, -7.5, 7.0, 20.0, 2.0, 16.0, 6.0, 7.0, 9.0, 0.35, 0.45, 0.20};
    }

    return {-18.0, -7.5, 7.0, 20.0, 2.0, 16.0, 6.0, 7.0, 9.0, 0.35, 0.45, 0.20};
}

double rangeScore(double value, double minGood, double maxGood, double softMargin) noexcept
{
    if (!std::isfinite(value))
        return 0.0;
    if (value >= minGood && value <= maxGood)
        return 100.0;
    const double distance = value < minGood ? minGood - value : value - maxGood;
    return std::clamp(100.0 - (distance / std::max(softMargin, 0.001)) * 50.0, 0.0, 100.0);
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

    // Universal technical safety: never relaxed by genre or era.
    double technical = 100.0;
    if (m.nonFiniteSamples > 0) technical = 0.0;
    if (m.clippedSamples > 0) technical -= std::min(30.0, 15.0 + 5.0 * std::log10(1.0 + static_cast<double>(m.clippedSamples)));
    if (m.truePeakDbtp > 0.0) technical -= std::min(40.0, 20.0 + m.truePeakDbtp * 10.0);
    if (m.correlation < 0.0) technical -= std::min(30.0, -m.correlation * 30.0);
    if (m.monoCompatibilityDb < -3.0) technical -= std::min(30.0, (-3.0 - m.monoCompatibilityDb) * 5.0);
    if (std::abs(m.lrBalanceDb) > 3.0) technical -= std::min(15.0, (std::abs(m.lrBalanceDb) - 3.0) * 3.0);
    if (m.dcOffsetLeftDbfs > -50.0 || m.dcOffsetRightDbfs > -50.0) technical -= 10.0;
    technical = std::clamp(technical, 0.0, 100.0);

    const Profile p = profileFor(mode, genre, era);
    const double loudnessScore = rangeScore(m.integratedLufs, p.loudnessMin, p.loudnessMax, p.loudnessMargin);
    const double plrScore = rangeScore(m.plrDb, p.plrMin, p.plrMax, p.plrMargin);
    const double lraScore = rangeScore(m.lraLu, p.lraMin, p.lraMax, p.lraMargin);
    double style = p.loudnessWeight * loudnessScore + p.plrWeight * plrScore + p.lraWeight * lraScore;

    // Tonal balance is still intentionally weak until per-genre distributions are
    // calibrated from raw reference data. Only pathological global skews are flagged.
    const double tonalTotal = m.tonalPercent[0] + m.tonalPercent[1] + m.tonalPercent[2] + m.tonalPercent[3];
    if (tonalTotal > 99.0 && tonalTotal < 101.0)
    {
        const bool extremeLow = m.tonalPercent[0] > 70.0;
        const bool extremeHigh = m.tonalPercent[3] > 50.0;
        if (extremeLow || extremeHigh)
            style = std::max(0.0, style - 5.0);
    }

    // Delivery compatibility models normalisation/transcoding risk, not artistry.
    // Spotify currently normalises Normal playback to -14 LUFS, recommends <=-1 dBTP,
    // and <=-2 dBTP for masters louder than -14 LUFS. A loud but clean master may
    // therefore retain an excellent style score while receiving a delivery warning.
    double delivery = 100.0;
    const bool louderThanStreamingReference = m.integratedLufs > -14.0;
    const double recommendedTp = louderThanStreamingReference ? -2.0 : -1.0;
    if (m.truePeakDbtp > recommendedTp)
        delivery -= std::min(45.0, 10.0 + (m.truePeakDbtp - recommendedTp) * 20.0);
    if (louderThanStreamingReference)
        delivery -= std::min(20.0, (m.integratedLufs + 14.0) * 2.5);
    delivery = std::clamp(delivery, 0.0, 100.0);

    // Technical faults cap the overall result and cannot be averaged away.
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
