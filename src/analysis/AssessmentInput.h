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

        // LIVE is provisional, but once enough programme context exists it
        // uses the same cumulative whole-programme metrics as FINAL. This
        // makes the running verdict converge naturally toward the definitive
        // result instead of introducing PLR/LRA only when FINALIZE is pressed.
        const double integrated = engine.calculateIntegratedLufs();
        const double plr = engine.calculatePlrDb();
        const double lra = engine.calculateLoudnessRangeLu();
        const bool minimumProgrammeContext = engine.hasProgrammeContext();

        m.integratedLufs = integrated;
        m.truePeakDbtp = engine.truePeakDbtp();
        m.plrDb = plr;
        m.lraLu = lra;
        m.crestFactorDb = engine.crestFactorDb();
        m.correlation = engine.correlation();
        m.monoCompatibilityDb = engine.monoCompatibilityDb();
        m.worstLocalCorrelation = engine.worstLocalCorrelation();
        m.worstLocalMonoCompatibilityDb = engine.worstLocalMonoCompatibilityDb();
        m.negativeCorrelationPercent = engine.negativeCorrelationPercent();
        m.lrBalanceDb = engine.lrBalanceDb();
        m.dcOffsetLeftDbfs = engine.dcOffsetLeftDbfs();
        m.dcOffsetRightDbfs = engine.dcOffsetRightDbfs();
        m.clippedSamples = engine.clippedSampleCount();
        m.nonFiniteSamples = engine.nonFiniteSampleCount();
        m.tonalPercent = {{engine.lowBandPercent(),engine.lowMidBandPercent(),engine.highMidBandPercent(),engine.highBandPercent()}};
        m.loudnessAvailable = minimumProgrammeContext
            && std::isfinite(integrated)
            && integrated > -999.0;
        m.plrAvailable = m.loudnessAvailable
            && std::isfinite(plr);
        m.lraAvailable = minimumProgrammeContext
            && std::isfinite(lra);
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
        m.worstLocalCorrelation = snapshot.worstLocalCorrelation;
        m.worstLocalMonoCompatibilityDb = snapshot.worstLocalMonoCompatibilityDb;
        m.negativeCorrelationPercent = snapshot.negativeCorrelationPercent;
        m.lrBalanceDb = snapshot.lrBalanceDb;
        m.dcOffsetLeftDbfs = snapshot.dcOffsetLeftDbfs;
        m.dcOffsetRightDbfs = snapshot.dcOffsetRightDbfs;
        m.clippedSamples = snapshot.clippedSampleCount;
        m.nonFiniteSamples = snapshot.nonFiniteSampleCount;
        m.tonalPercent = snapshot.tonalPercent;

        // FINAL must use historical programme sufficiency, not the current
        // Short-Term window. Otherwise several seconds of silence before the
        // user presses FINALIZE can incorrectly turn a valid full-song result
        // into N/A even though the cumulative programme analysis is intact.
        const bool minimumProgrammeContext = snapshot.valid && snapshot.programmeContext;
        m.loudnessAvailable = minimumProgrammeContext
            && std::isfinite(snapshot.integratedLufs)
            && snapshot.integratedLufs > -999.0;
        m.plrAvailable = m.loudnessAvailable
            && std::isfinite(snapshot.plrDb);
        m.lraAvailable = minimumProgrammeContext
            && std::isfinite(snapshot.loudnessRangeLu);
        m.provisional = false;
        return m;
    }
};
}
