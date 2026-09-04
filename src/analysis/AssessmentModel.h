#pragma once

#include <array>
#include <cstdint>

namespace Mixorator::Analysis
{
enum class AnalysisMode : std::uint8_t { Mix, Master };
enum class Era : std::uint8_t { Modern, Vintage };
enum class Genre : std::uint8_t
{
    Rock,
    Metal,
    Pop,
    Techno,
    HouseEdm,
    HipHopTrap,
    ElectronicAmbient,
    AcousticFolk,
    Jazz,
    Classical,
    Cinematic,
    General
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
    double lrBalanceDb {0.0};
    double dcOffsetLeftDbfs {-1000.0};
    double dcOffsetRightDbfs {-1000.0};
    std::uint64_t clippedSamples {0};
    std::uint64_t nonFiniteSamples {0};
    std::array<double, 4> tonalPercent {{0.0, 0.0, 0.0, 0.0}};
};

struct Assessment
{
    double technicalScore {100.0};
    double styleScore {100.0};
    double deliveryScore {100.0};
    double overallScore {100.0};
    Verdict technicalVerdict {Verdict::Excellent};
    Verdict styleVerdict {Verdict::Excellent};
    Verdict deliveryVerdict {Verdict::Excellent};
    Verdict overallVerdict {Verdict::Excellent};
};

class AssessmentModel
{
public:
    static Assessment evaluate(const Metrics& metrics, AnalysisMode mode, Genre genre, Era era) noexcept;
};
}
