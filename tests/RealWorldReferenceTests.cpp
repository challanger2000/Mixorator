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

bool approx(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a-b) <= tolerance;
}

Mixorator::Analysis::Metrics cleanReference()
{
    Mixorator::Analysis::Metrics m;
    m.correlation = 0.75;
    m.monoCompatibilityDb = -0.5;
    m.worstLocalCorrelation = 0.75;
    m.worstLocalMonoCompatibilityDb = -0.5;
    m.negativeCorrelationPercent = 0.0;
    m.lrBalanceDb = 0.0;
    m.dcOffsetLeftDbfs = -90.0;
    m.dcOffsetRightDbfs = -90.0;
    m.clippedSamples = 0;
    m.nonFiniteSamples = 0;
    return m;
}
}

int main()
{
    using namespace Mixorator::Analysis;

    // Reference A: "Menschlichkeit". Loud and controlled, but not an
    // extreme loudness/PLR combination. Density logic must leave technical
    // quality untouched. Its -0.5 dBTP leaves limited codec headroom, so the
    // service-independent streaming assessment should be GOOD, not ATTENTION.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -11.4;
        m.truePeakDbtp = -0.5;
        m.plrDb = 10.8;
        m.lraLu = 3.6;
        m.correlation = 0.73;
        m.monoCompatibilityDb = -0.6;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.technicalScore < 99.0)
            return fail("Menschlichkeit-like reference was penalized for healthy master density");
        if (a.streamingDeliveryVerdict != Verdict::Good)
            return fail("Menschlichkeit-like reference was over-penalized for streaming delivery");
    }

    // Reference B: "Aloha Oe". The master is technically clean at the peak,
    // but -5.9 LUFS together with 5.5 dB PLR is an extreme density trade-off.
    // It may remain an excellent style match, but technical quality must not
    // claim EXCELLENT. Loudness itself must not create a streaming warning.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -5.9;
        m.truePeakDbtp = -0.4;
        m.plrDb = 5.5;
        m.lraLu = 3.8;
        m.correlation = 0.81;
        m.monoCompatibilityDb = -0.4;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.technicalVerdict != Verdict::Good || a.technicalScore < 75.0 || a.technicalScore >= 90.0)
            return fail("Aloha-Oe-like extreme master density was not downgraded to GOOD technical quality");
        if (a.styleVerdict != Verdict::Excellent)
            return fail("Aloha-Oe-like intentional density incorrectly damaged Metal style match");
        if (a.pcmDeliveryVerdict != Verdict::Excellent)
            return fail("Aloha-Oe-like density incorrectly contaminated master delivery safety");
        if (a.streamingDeliveryVerdict != Verdict::Good)
            return fail("Aloha-Oe-like loudness was incorrectly treated as a universal streaming fault");
    }

    // The density trade-off belongs to finished-master assessment only.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -5.9;
        m.truePeakDbtp = -0.4;
        m.plrDb = 5.5;
        m.lraLu = 3.8;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Mix, Genre::Metal, Era::Modern);
        if (a.technicalVerdict != Verdict::Excellent)
            return fail("Finished-master density rule leaked into MIX technical quality");
    }

    // Reference C: "Lost In Shadow". Dynamics are healthy, but +1.7 dBTP is
    // the actual technical fault. It must remain a clear streaming/transcoding
    // risk independent of programme loudness.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -9.2;
        m.truePeakDbtp = 1.7;
        m.plrDb = 10.8;
        m.lraLu = 2.4;
        m.correlation = 0.79;
        m.monoCompatibilityDb = -0.5;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Electronic, Era::Modern);
        if (a.technicalVerdict != Verdict::Attention)
            return fail("Lost-In-Shadow-like true-peak fault was not kept distinct from density scoring");
        if (a.pcmDeliveryVerdict != Verdict::Attention)
            return fail("Lost-In-Shadow-like +1.7 dBTP did not lower master delivery assessment");
        if (a.streamingDeliveryVerdict != Verdict::Attention)
            return fail("Lost-In-Shadow-like +1.7 dBTP did not remain a streaming/transcoding warning");
    }

    // Streaming robustness must not silently become a Spotify -14 LUFS
    // compliance score. Equal peak headroom must produce equal streaming
    // quality regardless of artistic loudness.
    {
        Metrics loud = cleanReference();
        loud.integratedLufs = -6.0;
        loud.truePeakDbtp = -0.8;
        loud.plrDb = 9.0;
        loud.lraLu = 4.0;
        Metrics quiet = loud;
        quiet.integratedLufs = -16.0;
        const auto loudA = AssessmentModel::evaluate(loud, AnalysisMode::Master, Genre::General, Era::Modern);
        const auto quietA = AssessmentModel::evaluate(quiet, AnalysisMode::Master, Genre::General, Era::Modern);
        if (!approx(loudA.streamingDeliveryScore, quietA.streamingDeliveryScore))
            return fail("Streaming robustness still depends on a universal loudness target");
    }

    // Dynamic genres must not be punished technically merely for having high
    // PLR and LRA. These are musical traits, not safety faults.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -18.0;
        m.truePeakDbtp = -2.0;
        m.plrDb = 22.0;
        m.lraLu = 14.0;
        const auto classical = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Classical, Era::Modern);
        if (classical.technicalVerdict != Verdict::Excellent)
            return fail("Healthy dynamic classical master was penalized technically for dynamics");
        if (classical.styleVerdict != Verdict::Excellent)
            return fail("Healthy dynamic classical master was not recognized as an excellent style match");
    }

    // Real orchestral stereo can contain a few strongly decorrelated 100 ms
    // windows from room/instrument transients. The overall programme remains
    // mono-safe, so a ~4% negative-window prevalence must not turn the master
    // into a technical warning solely because of the single worst window.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -13.7;
        m.truePeakDbtp = -0.4;
        m.plrDb = 13.2;
        m.lraLu = 14.2;
        m.correlation = 0.34;
        m.monoCompatibilityDb = -1.7;
        m.worstLocalCorrelation = -0.53;
        m.worstLocalMonoCompatibilityDb = -6.23;
        m.negativeCorrelationPercent = 4.26;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Classical, Era::Modern);
        if (a.technicalVerdict != Verdict::Excellent)
            return fail("Transient orchestral stereo outliers were over-weighted in technical quality");
    }

    // The same local severity becomes a genuine technical problem when it is
    // prevalent. Sustained phase/decorrelation must therefore still produce a
    // clear technical downgrade; the prevalence weighting must not hide it.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -13.7;
        m.truePeakDbtp = -1.2;
        m.plrDb = 13.2;
        m.lraLu = 8.0;
        m.correlation = 0.20;
        m.monoCompatibilityDb = -2.0;
        m.worstLocalCorrelation = -0.53;
        m.worstLocalMonoCompatibilityDb = -6.23;
        m.negativeCorrelationPercent = 35.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::General, Era::Modern);
        if (a.technicalVerdict == Verdict::Excellent)
            return fail("Sustained stereo phase problem was hidden by prevalence weighting");
    }

    // The exact same safe signal may be stylistically unusual for a modern
    // dense genre, but technical integrity must remain identical.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -18.0;
        m.truePeakDbtp = -2.0;
        m.plrDb = 22.0;
        m.lraLu = 14.0;
        const auto classical = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Classical, Era::Modern);
        const auto techno = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Techno, Era::Modern);
        if (!approx(classical.technicalScore, techno.technicalScore))
            return fail("Genre selection changed technical integrity for identical audio");
        if (techno.styleScore >= classical.styleScore)
            return fail("Highly dynamic programme did not distinguish classical from modern techno style context");
    }

    // Era is a stylistic context only. A vintage-like cinematic master should
    // fit the vintage profile better than the modern one, while all objective
    // technical and delivery scores remain exactly unchanged.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -18.5;
        m.truePeakDbtp = -1.5;
        m.plrDb = 27.0;
        m.lraLu = 20.0;
        const auto vintage = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Cinematic, Era::Vintage);
        const auto modern = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Cinematic, Era::Modern);
        if (vintage.styleScore <= modern.styleScore)
            return fail("Vintage cinematic context did not prefer vintage-like dynamics");
        if (!approx(vintage.technicalScore, modern.technicalScore) ||
            !approx(vintage.pcmDeliveryScore, modern.pcmDeliveryScore) ||
            !approx(vintage.streamingDeliveryScore, modern.streamingDeliveryScore))
            return fail("Era selection changed objective technical or delivery quality");
    }

    // The inverse must also hold: a loud, compact modern pop master should fit
    // Modern better than Vintage without changing technical integrity.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -7.0;
        m.truePeakDbtp = -1.2;
        m.plrDb = 7.0;
        m.lraLu = 3.0;
        const auto modern = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Pop, Era::Modern);
        const auto vintage = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Pop, Era::Vintage);
        if (modern.styleScore <= vintage.styleScore)
            return fail("Modern pop context did not prefer modern-like density");
        if (!approx(modern.technicalScore, vintage.technicalScore))
            return fail("Era selection changed technical integrity for modern pop");
    }

    // MIX has intentionally era-neutral profiles at present. Era selection
    // must therefore not alter either technical or style scoring in MIX mode.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -18.0;
        m.truePeakDbtp = -3.0;
        m.plrDb = 15.0;
        m.lraLu = 8.0;
        const auto modern = AssessmentModel::evaluate(m, AnalysisMode::Mix, Genre::Rock, Era::Modern);
        const auto vintage = AssessmentModel::evaluate(m, AnalysisMode::Mix, Genre::Rock, Era::Vintage);
        if (!approx(modern.technicalScore, vintage.technicalScore) ||
            !approx(modern.styleScore, vintage.styleScore))
            return fail("Era unexpectedly changed MIX assessment");
    }

    // Jazz and acoustic material should tolerate healthy headroom and broad
    // macro-dynamics without being labelled technically weak.
    {
        Metrics jazz = cleanReference();
        jazz.integratedLufs = -16.0;
        jazz.truePeakDbtp = -1.5;
        jazz.plrDb = 17.0;
        jazz.lraLu = 10.0;
        const auto j = AssessmentModel::evaluate(jazz, AnalysisMode::Master, Genre::Jazz, Era::Modern);
        if (j.technicalVerdict != Verdict::Excellent || j.styleVerdict == Verdict::Critical)
            return fail("Healthy jazz-like dynamics were misclassified");

        Metrics folk = cleanReference();
        folk.integratedLufs = -15.0;
        folk.truePeakDbtp = -1.5;
        folk.plrDb = 16.0;
        folk.lraLu = 9.0;
        const auto f = AssessmentModel::evaluate(folk, AnalysisMode::Master, Genre::AcousticFolk, Era::Modern);
        if (f.technicalVerdict != Verdict::Excellent || f.styleVerdict == Verdict::Critical)
            return fail("Healthy acoustic/folk-like dynamics were misclassified");
    }

    // Very loud material can still be technically safe at the peak. Density
    // may reduce technical quality to GOOD, but must not spill into delivery
    // safety when true peak remains controlled.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -6.3;
        m.truePeakDbtp = -1.2;
        m.plrDb = 5.8;
        m.lraLu = 3.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::HouseEdm, Era::Modern);
        if (a.technicalVerdict != Verdict::Good)
            return fail("Safe but strongly dense EDM master was not classified as GOOD technical quality");
        if (a.pcmDeliveryVerdict != Verdict::Excellent || a.streamingDeliveryVerdict != Verdict::Excellent)
            return fail("Safe true-peak headroom was contaminated by artistic loudness/density");
    }

    // A quiet master with positive true peak is still unsafe. Loudness must
    // never hide inter-sample overs.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -18.0;
        m.truePeakDbtp = 0.8;
        m.plrDb = 15.0;
        m.lraLu = 8.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Jazz, Era::Modern);
        if (a.technicalVerdict == Verdict::Excellent || a.pcmDeliveryVerdict == Verdict::Excellent ||
            a.streamingDeliveryVerdict == Verdict::Excellent)
            return fail("Quiet programme incorrectly hid a positive true-peak fault");
    }

    // Real-world decoded MP3 case (Ricky-Nelson-like): a modest positive
    // true peak together with a limited number of decoded sample overs is one
    // ceiling-risk family. Streaming must warn, but must not double-count both
    // manifestations into CRITICAL.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -10.6;
        m.truePeakDbtp = 0.5;
        m.plrDb = 10.9;
        m.lraLu = 4.9;
        m.correlation = 0.97;
        m.monoCompatibilityDb = -0.1;
        m.clippedSamples = 247;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Pop, Era::Vintage);
        if (a.streamingDeliveryVerdict != Verdict::Attention)
            return fail("Ricky-like decoded overs were double-counted into the wrong streaming verdict");
    }

    // Real-world hard-ceiling EDM case (Glow-like): thousands of samples can
    // sit at full scale while reconstructed true peak is positive. Technical
    // and master-quality assessment may remain severe, but streaming should
    // describe the single ceiling/transcoding risk once, not twice.
    {
        Metrics m = cleanReference();
        m.integratedLufs = -8.7;
        m.truePeakDbtp = 0.6;
        m.plrDb = 9.2;
        m.lraLu = 8.6;
        m.correlation = 0.85;
        m.monoCompatibilityDb = -0.3;
        m.clippedSamples = 2000;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::HouseEdm, Era::Modern);
        if (a.technicalVerdict != Verdict::Critical)
            return fail("Glow-like hard ceiling was not kept severe in technical quality");
        if (a.streamingDeliveryVerdict != Verdict::Attention)
            return fail("Glow-like ceiling risk was double-counted in streaming assessment");
    }

    std::cout << "Real-world reference assessment tests passed.\n";
    return 0;
}
