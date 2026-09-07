#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Mixorator::Analysis
{
enum class AnalysisMode : std::uint8_t { Mix, Master };
enum class Era : std::uint8_t { Modern, Vintage };
enum class Genre : std::uint8_t
{
    Rock, Metal, Pop, Techno, HouseEdm, DrumAndBass, HipHopTrap, RnBSoul,
    Electronic, Ambient, AcousticFolk, Jazz, Classical, Cinematic, General
};

enum class Verdict : std::uint8_t { Excellent, Good, Attention, Critical, Unusual, InsufficientData };

struct Metrics
{
    double integratedLufs {-1000.0};
    double truePeakDbtp {-1000.0};
    double plrDb {0.0};
    double lraLu {0.0};
    double crestFactorDb {0.0};
    double correlation {1.0};
    double monoCompatibilityDb {0.0};
    double worstLocalCorrelation {1.0};
    double worstLocalMonoCompatibilityDb {0.0};
    double negativeCorrelationPercent {0.0};
    double lrBalanceDb {0.0};
    double dcOffsetLeftDbfs {-1000.0};
    double dcOffsetRightDbfs {-1000.0};
    std::uint64_t clippedSamples {0};
    std::uint64_t nonFiniteSamples {0};
    std::array<double, 4> tonalPercent {{0.0, 0.0, 0.0, 0.0}};
    std::array<double, 8> detailedTonalPercent {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};

    bool loudnessAvailable {true};
    bool plrAvailable {true};
    bool lraAvailable {true};
    bool provisional {false};
};

struct Assessment
{
    double technicalScore {100.0};
    double styleScore {100.0};
    double pcmDeliveryScore {100.0};
    double streamingDeliveryScore {100.0};
    double overallScore {100.0};
    Verdict technicalVerdict {Verdict::Excellent};
    Verdict styleVerdict {Verdict::Excellent};
    Verdict pcmDeliveryVerdict {Verdict::Excellent};
    Verdict streamingDeliveryVerdict {Verdict::Excellent};
    Verdict overallVerdict {Verdict::Excellent};
    double streamingGainDb {0.0};
    bool provisional {false};
};

// Relative spectral shape for calibration/diagnosis. Values are dB-like energy
// ratios (10*log10), not absolute band percentages. A small floor keeps silent
// upper bands finite without materially changing normal musical measurements.
struct TonalRatioFeatures
{
    double subVsBassDb {0.0};
    double lowMidVsMidDb {0.0};
    double presenceVsMidDb {0.0};
    double upperPresenceVsPresenceDb {0.0};
    double brillianceVsUpperPresenceDb {0.0};
    double airVsBrillianceDb {0.0};
    double lowVsMidDb {0.0};
    double highVsMidDb {0.0};
};

class AssessmentModel
{
public:
    static Assessment evaluate(const Metrics& metrics, AnalysisMode mode, Genre genre, Era era) noexcept;

    static bool detailedTonalDataAvailable(const Metrics& metrics) noexcept
    {
        double total = 0.0;
        for (double value : metrics.detailedTonalPercent)
        {
            if (!std::isfinite(value) || value < 0.0)
                return false;
            total += value;
        }
        return total > 99.0 && total < 101.0;
    }

    static TonalRatioFeatures tonalRatioFeatures(const Metrics& metrics) noexcept
    {
        if (!detailedTonalDataAvailable(metrics))
            return {};

        constexpr double floor = 1.0e-6;
        const auto ratioDb = [](double numerator, double denominator) noexcept
        {
            constexpr double localFloor = 1.0e-6;
            return 10.0 * std::log10(std::max(numerator, localFloor) /
                                     std::max(denominator, localFloor));
        };

        const auto& b = metrics.detailedTonalPercent;
        TonalRatioFeatures f;
        f.subVsBassDb = ratioDb(b[0], b[1]);
        f.lowMidVsMidDb = ratioDb(b[2], b[3]);
        f.presenceVsMidDb = ratioDb(b[4], b[3]);
        f.upperPresenceVsPresenceDb = ratioDb(b[5], b[4]);
        f.brillianceVsUpperPresenceDb = ratioDb(b[6], b[5]);
        f.airVsBrillianceDb = ratioDb(b[7], b[6]);
        f.lowVsMidDb = ratioDb(b[0] + b[1], b[2] + b[3]);
        f.highVsMidDb = ratioDb(b[4] + b[5] + b[6] + b[7], b[2] + b[3]);
        (void) floor;
        return f;
    }

    // Legacy experimental raw-band calibration score. Kept isolated from
    // evaluate() while ratio-based calibration is validated against references.
    static double detailedTonalScore(const Metrics& metrics, Genre genre) noexcept
    {
        if (!detailedTonalDataAvailable(metrics))
            return 0.0;

        struct DetailedProfile
        {
            std::array<double, 8> min;
            std::array<double, 8> max;
            std::array<double, 8> margin;
        };

        const auto profileFor = [](Genre g) noexcept -> DetailedProfile
        {
            switch (g)
            {
                case Genre::Metal:
                    return {{{1,10,7,18,10,4,1,.1}}, {{16,32,22,42,30,19,11,6}}, {{8,10,8,12,10,8,6,4}}};
                case Genre::Rock:
                    return {{{1,9,8,20,8,4,1,.1}}, {{15,30,24,45,27,18,11,7}}, {{8,10,8,12,10,8,6,4}}};
                case Genre::Pop:
                    return {{{2,10,7,18,8,4,1,.1}}, {{18,32,22,42,26,18,12,8}}, {{9,10,8,12,10,8,6,5}}};
                case Genre::Techno:
                case Genre::HouseEdm:
                    return {{{8,14,4,10,5,2,1,.1}}, {{32,38,18,32,22,15,10,6}}, {{12,12,7,10,9,7,5,4}}};
                case Genre::DrumAndBass:
                    return {{{10,12,4,10,5,2,1,.1}}, {{36,36,18,30,22,15,10,6}}, {{13,12,7,10,9,7,5,4}}};
                case Genre::HipHopTrap:
                    return {{{9,14,4,10,4,2,.5,.1}}, {{38,40,18,30,20,13,9,5}}, {{14,13,7,10,8,6,5,4}}};
                case Genre::RnBSoul:
                    return {{{2,10,8,20,5,3,.5,.1}}, {{20,34,26,48,22,15,10,7}}, {{9,11,9,13,9,7,5,4}}};
                case Genre::Electronic:
                    return {{{3,9,5,14,5,2,.5,.1}}, {{28,36,24,42,25,18,13,9}}, {{11,12,9,12,10,8,6,5}}};
                case Genre::Ambient:
                    return {{{1,6,6,16,4,2,.5,.1}}, {{24,30,28,50,24,18,14,10}}, {{10,11,10,14,10,8,7,6}}};
                case Genre::AcousticFolk:
                    return {{{.01,4,8,5,.4,.1,.03,.005}}, {{10,28,75,58,28,17,11,8}}, {{6,10,18,18,10,8,6,5}}};
                case Genre::Jazz:
                    return {{{.2,5,10,25,7,3,.5,.1}}, {{11,25,32,55,28,17,11,8}}, {{6,9,10,15,10,7,5,4}}};
                case Genre::Classical:
                    return {{{.1,4,8,24,6,3,.5,.1}}, {{10,23,32,58,28,18,12,9}}, {{6,9,10,16,10,8,6,5}}};
                case Genre::Cinematic:
                    return {{{3,8,6,16,5,2,.5,.1}}, {{30,32,28,48,26,18,13,9}}, {{12,11,10,14,10,8,6,5}}};
                case Genre::General:
                default:
                    return {{{.1,4,5,12,3,1,.2,.05}}, {{35,42,34,58,32,22,16,12}}, {{14,14,12,16,12,10,8,6}}};
            }
        };

        const auto rangeScore = [](double value, double lo, double hi, double margin) noexcept
        {
            if (value >= lo && value <= hi)
                return 100.0;
            const double distance = value < lo ? lo - value : value - hi;
            return std::clamp(100.0 - distance / std::max(margin, 0.001) * 50.0, 0.0, 100.0);
        };

        const DetailedProfile profile = profileFor(genre);
        constexpr std::array<double, 8> weights {{.13,.18,.14,.20,.15,.09,.07,.04}};
        double score = 0.0;
        for (std::size_t i = 0; i < metrics.detailedTonalPercent.size(); ++i)
            score += weights[i] * rangeScore(metrics.detailedTonalPercent[i], profile.min[i], profile.max[i], profile.margin[i]);
        return std::clamp(score, 0.0, 100.0);
    }
};
}
