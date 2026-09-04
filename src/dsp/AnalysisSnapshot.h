#pragma once

#include "AnalysisEngine.h"

#include <array>
#include <cstdint>

namespace Mixorator::DSP
{
struct AnalysisSnapshot
{
    bool valid {false};

    double samplePeakDbfs {-1000.0};
    double truePeakDbtp {-1000.0};
    double rmsDbfs {-1000.0};
    double crestFactorDb {0.0};
    double momentaryLufs {-1000.0};
    double shortTermLufs {-1000.0};
    double integratedLufs {-1000.0};
    double loudnessRangeLu {0.0};
    double plrDb {0.0};

    double lrBalanceDb {0.0};
    double correlation {1.0};
    double stereoWidthDb {-1000.0};
    double monoCompatibilityDb {0.0};
    double worstLocalCorrelation {1.0};
    double worstLocalMonoCompatibilityDb {0.0};
    double negativeCorrelationPercent {0.0};

    double dcOffsetLeftDbfs {-1000.0};
    double dcOffsetRightDbfs {-1000.0};
    std::uint64_t clippedSampleCount {0};
    std::uint64_t nonFiniteSampleCount {0};

    std::array<double, 4> tonalPercent {{0.0, 0.0, 0.0, 0.0}};

    static AnalysisSnapshot capture(const AnalysisEngine& engine) noexcept
    {
        AnalysisSnapshot s;
        s.valid = true;
        s.samplePeakDbfs = engine.samplePeakDbfs();
        s.truePeakDbtp = engine.truePeakDbtp();
        s.rmsDbfs = engine.rmsDbfs();
        s.crestFactorDb = engine.crestFactorDb();
        s.momentaryLufs = engine.momentaryLufs();
        s.shortTermLufs = engine.shortTermLufs();
        s.integratedLufs = engine.calculateIntegratedLufs();
        s.loudnessRangeLu = engine.calculateLoudnessRangeLu();
        s.plrDb = engine.calculatePlrDb();
        s.lrBalanceDb = engine.lrBalanceDb();
        s.correlation = engine.correlation();
        s.stereoWidthDb = engine.stereoWidthDb();
        s.monoCompatibilityDb = engine.monoCompatibilityDb();
        s.worstLocalCorrelation = engine.worstLocalCorrelation();
        s.worstLocalMonoCompatibilityDb = engine.worstLocalMonoCompatibilityDb();
        s.negativeCorrelationPercent = engine.negativeCorrelationPercent();
        s.dcOffsetLeftDbfs = engine.dcOffsetLeftDbfs();
        s.dcOffsetRightDbfs = engine.dcOffsetRightDbfs();
        s.clippedSampleCount = engine.clippedSampleCount();
        s.nonFiniteSampleCount = engine.nonFiniteSampleCount();
        s.tonalPercent = {{
            engine.lowBandPercent(),
            engine.lowMidBandPercent(),
            engine.highMidBandPercent(),
            engine.highBandPercent()
        }};
        return s;
    }
};
}
