#include "dsp/AnalysisEngine.h"
#include "analysis/AssessmentModel.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
constexpr double kPi = 3.1415926535897932384626433832795;

bool approx(double a, double b, double tol) { return std::abs(a - b) <= tol; }

int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

void processStereo(Mixorator::DSP::AnalysisEngine& e,
                   const std::vector<double>& left,
                   const std::vector<double>& right,
                   int blockSize = 256)
{
    const int n = static_cast<int>(left.size());
    for (int pos = 0; pos < n; pos += blockSize)
    {
        const int count = std::min(blockSize, n - pos);
        double* channels[2] = {
            const_cast<double*>(left.data() + pos),
            const_cast<double*>(right.data() + pos)
        };
        e.process(channels, 2, count);
    }
}

std::vector<double> sine(double sr, double hz, double seconds, double amp, double phase = 0.0)
{
    const std::size_t n = static_cast<std::size_t>(std::llround(sr * seconds));
    std::vector<double> v(n);
    for (std::size_t i = 0; i < n; ++i)
        v[i] = amp * std::sin(2.0 * kPi * hz * static_cast<double>(i) / sr + phase);
    return v;
}

Mixorator::Analysis::Metrics cleanMetrics()
{
    Mixorator::Analysis::Metrics m;
    m.integratedLufs = -12.0;
    m.truePeakDbtp = -2.1;
    m.plrDb = 10.0;
    m.lraLu = 6.0;
    m.crestFactorDb = 10.0;
    m.correlation = 0.7;
    m.monoCompatibilityDb = -0.5;
    m.lrBalanceDb = 0.2;
    m.dcOffsetLeftDbfs = -90.0;
    m.dcOffsetRightDbfs = -90.0;
    return m;
}
}

int main()
{
    using Mixorator::DSP::AnalysisEngine;
    using namespace Mixorator::Analysis;

    constexpr double sr = 48000.0;

    // 1) In-phase stereo: correlation +1, no mono loss, no balance error.
    {
        AnalysisEngine e;
        e.prepare(sr);
        auto l = sine(sr, 1000.0, 4.0, 0.5);
        auto r = l;
        processStereo(e, l, r);

        if (e.truePeakDbtp() + 1e-9 < e.samplePeakDbfs()) return fail("True Peak fell below Sample Peak");
        if (!approx(e.correlation(), 1.0, 1e-6)) return fail("In-phase correlation is not +1");
        if (!approx(e.lrBalanceDb(), 0.0, 1e-6)) return fail("Equal stereo channels are not balanced");
        if (!approx(e.monoCompatibilityDb(), 0.0, 1e-6)) return fail("In-phase mono compatibility is not 0 dB");
        if (std::abs(e.calculateLoudnessRangeLu()) > 0.2) return fail("Constant programme LRA is not approximately 0 LU");
    }

    // 2) Anti-phase stereo: correlation -1 and severe mono cancellation.
    {
        AnalysisEngine e;
        e.prepare(sr);
        auto l = sine(sr, 1000.0, 4.0, 0.5);
        auto r = l;
        for (auto& x : r) x = -x;
        processStereo(e, l, r);

        if (!approx(e.correlation(), -1.0, 1e-6)) return fail("Anti-phase correlation is not -1");
        if (e.monoCompatibilityDb() > -100.0) return fail("Anti-phase mono cancellation was not detected");
    }

    // 3) True-peak stress invariant.
    {
        AnalysisEngine e;
        e.prepare(sr);
        auto l = sine(sr, 12000.0, 1.0, 0.95, kPi / 4.0);
        auto r = l;
        processStereo(e, l, r);
        if (e.truePeakDbtp() + 1e-9 < e.samplePeakDbfs()) return fail("True Peak invariant failed on intersample stress signal");
    }

    // 4) Tonal classification: 100 Hz -> low band.
    {
        AnalysisEngine e;
        e.prepare(sr);
        auto l = sine(sr, 100.0, 2.0, 0.5);
        auto r = l;
        processStereo(e, l, r);
        if (e.lowBandPercent() < 90.0) return fail("100 Hz tone was not classified as low-band energy");
    }

    // 5) Tonal classification: 4 kHz -> high-mid band.
    {
        AnalysisEngine e;
        e.prepare(sr);
        auto l = sine(sr, 4000.0, 2.0, 0.5);
        auto r = l;
        processStereo(e, l, r);
        if (e.highMidBandPercent() < 90.0) return fail("4 kHz tone was not classified as high-mid energy");
    }

    // 6) A plausible modern metal master can be loud without becoming a technical fault.
    {
        Metrics m = cleanMetrics();
        m.integratedLufs = -8.0;
        m.truePeakDbtp = -2.1;
        m.plrDb = 7.0;
        m.lraLu = 4.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.technicalScore < 99.0) return fail("Loud but safe metal master was technically penalized");
        if (a.styleScore < 85.0) return fail("Plausible modern metal master did not score strongly for style");
    }

    // 7) Delivery is independent: same musical master can be stylistically strong but receive streaming warnings.
    {
        Metrics m = cleanMetrics();
        m.integratedLufs = -8.0;
        m.truePeakDbtp = -1.0;
        m.plrDb = 7.0;
        m.lraLu = 4.0;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.styleScore < 85.0) return fail("Delivery warning contaminated style score");
        if (a.deliveryScore >= 90.0) return fail("Loud master with limited codec headroom did not receive delivery warning");
    }

    // 8) Era must affect style expectations but never technical safety.
    {
        Metrics m = cleanMetrics();
        m.integratedLufs = -8.0;
        m.truePeakDbtp = -2.1;
        m.plrDb = 7.0;
        m.lraLu = 4.0;
        const auto modern = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Rock, Era::Modern);
        const auto vintage = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Rock, Era::Vintage);
        if (!approx(modern.technicalScore, vintage.technicalScore, 1e-9)) return fail("Era altered universal technical safety");
        if (modern.styleScore <= vintage.styleScore) return fail("Modern/vintage Rock profiles are not meaningfully differentiated");
    }

    // 9) Classical should reward preserved dynamics over a hyper-limited profile.
    {
        Metrics dynamic = cleanMetrics();
        dynamic.integratedLufs = -18.0;
        dynamic.plrDb = 20.0;
        dynamic.lraLu = 15.0;
        Metrics crushed = dynamic;
        crushed.plrDb = 5.0;
        crushed.lraLu = 1.0;
        const auto good = AssessmentModel::evaluate(dynamic, AnalysisMode::Master, Genre::Classical, Era::Modern);
        const auto bad = AssessmentModel::evaluate(crushed, AnalysisMode::Master, Genre::Classical, Era::Modern);
        if (good.styleScore <= bad.styleScore + 20.0) return fail("Classical dynamics profile is not differentiated enough");
    }

    // 10) Mix and master are distinct states; a healthy mix should not be forced toward mastered loudness.
    {
        Metrics m = cleanMetrics();
        m.integratedLufs = -20.0;
        m.plrDb = 14.0;
        m.lraLu = 8.0;
        const auto mix = AssessmentModel::evaluate(m, AnalysisMode::Mix, Genre::Pop, Era::Modern);
        const auto master = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Pop, Era::Modern);
        if (mix.styleScore <= master.styleScore) return fail("MIX profile is not distinct from MASTER profile");
    }

    // 11) Critical technical faults must cap overall result regardless of style.
    {
        Metrics m = cleanMetrics();
        m.integratedLufs = -9.0;
        m.truePeakDbtp = 2.0;
        m.plrDb = 8.0;
        m.lraLu = 4.0;
        m.correlation = -1.0;
        m.monoCompatibilityDb = -1000.0;
        m.dcOffsetLeftDbfs = -20.0;
        m.dcOffsetRightDbfs = -20.0;
        m.clippedSamples = 100;
        const auto a = AssessmentModel::evaluate(m, AnalysisMode::Master, Genre::Metal, Era::Modern);
        if (a.technicalScore >= 50.0) return fail("Severe technical faults did not become critical");
        if (a.overallScore >= 50.0) return fail("Critical technical faults were averaged away in overall score");
    }

    // 12) Every genre/profile combination must produce finite scores for valid input.
    {
        Metrics m = cleanMetrics();
        for (int mode = 0; mode < 2; ++mode)
        for (int era = 0; era < 2; ++era)
        for (int genre = 0; genre <= static_cast<int>(Genre::General); ++genre)
        {
            const auto a = AssessmentModel::evaluate(
                m,
                static_cast<AnalysisMode>(mode),
                static_cast<Genre>(genre),
                static_cast<Era>(era));
            if (!std::isfinite(a.technicalScore) || !std::isfinite(a.styleScore) ||
                !std::isfinite(a.deliveryScore) || !std::isfinite(a.overallScore))
                return fail("A calibrated profile produced a non-finite score");
        }
    }

    std::cout << "All Mixorator deterministic tests passed.\n";
    return 0;
}
