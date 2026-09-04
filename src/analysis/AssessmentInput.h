#pragma once

#include "AssessmentModel.h"
#include "../dsp/AnalysisEngine.h"
#include "../dsp/AnalysisSnapshot.h"

#include <cmath>

namespace Mixorator::Analysis
{
struct AssessmentInput
{
    static Metrics fromLive(const DSP::AnalysisEngine& engine) noexcept
    {
        Metrics m;
        const double shortTerm = engine.shortTermLufs();
        m.integratedLufs = shortTerm;
        m.truePeakDbtp = engine.truePeakDbtp();
        m.plrDb = 0.0;
        m.lraLu = 0.0;
        m.crestFactorDb = engine.crestFactorDb();
        m.correlation = engine.correlation();
        m.monoCompatibilityDb = engine.monoCompatibilityDb();
        m.lrBalanceDb = engine.lrBalanceDb();
        m.dcOffsetLeftDbfs = engine.dcOffsetLeftDbfs();
        m.dcOffsetRightDbfs = engine.dcOffsetRightDbfs();
        m.clippedSamples = engine.clippedSampleCount();
        m.nonFiniteSamples = engine.nonFiniteSampleCount();
        m.tonalPercent = {{
            engine.lowBandPercent(),
            engine.lowMidBandPercent(),
            engine.highMidBandPercent(),
            engine.highBandPercent()
        }};
        m.loudnessAvailable = std::isfinite(shortTerm) && shortTerm > -999.0;
        m.plrAvailable = false;
        m.lraAvailable = false;
        m.provisional = true;
        return m;
    }

    static Metrics fromFinal(const DSP::AnalysisSnapshot& snapshot) noexcept
    {
        Metrics m;
        m.integratedLufs = snapshot.integratedLufs;
        m.truePeakDbtp = snapshot.truePeakDbtp;
        m.plrDb = snapshot.plrDb;
        m.lraLu = snapshot.loudnessRangeLu;
        m.crestFactorDb = snapshot.crestFactorDb;
        m.correlation = snapshot.correlation;
        m.monoCompatibilityDb = snapshot.monoCompatibilityDb;
        m.lrBalanceDb = snapshot.lrBalanceDb;
        m.dcOffsetLeftDbfs = snapshot.dcOffsetLeftDbfs;
        m.dcOffsetRightDbfs = snapshot.dcOffsetRightDbfs;
        m.clippedSamples = snapshot.clippedSampleCount;
        m.nonFiniteSamples = snapshot.nonFiniteSampleCount;
        m.tonalPercent = snapshot.tonalPercent;
        m.loudnessAvailable = snapshot.valid && std::isfinite(snapshot.integratedLufs);
        m.plrAvailable = snapshot.valid && std::isfinite(snapshot.plrDb);
        m.lraAvailable = snapshot.valid && std::isfinite(snapshot.loudnessRangeLu);
        m.provisional = false;
        return m;
    }
};
}
