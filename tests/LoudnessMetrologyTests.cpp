#include "dsp/AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
constexpr double kPi = 3.1415926535897932384626433832795;
int fail(const char* message) { std::cerr << "FAIL: " << message << '\n'; return 1; }
bool approx(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }

std::vector<double> sine(double sampleRate, double frequency, double seconds, double amplitude)
{
    std::vector<double> result(static_cast<std::size_t>(std::llround(sampleRate * seconds)));
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = amplitude * std::sin(2.0 * kPi * frequency * static_cast<double>(i) / sampleRate);
    return result;
}

void processStereo(Mixorator::DSP::AnalysisEngine& engine,
                   std::vector<double>& left,
                   std::vector<double>& right,
                   int blockSize = 257)
{
    for (int pos = 0; pos < static_cast<int>(left.size()); pos += blockSize)
    {
        const int count = std::min(blockSize, static_cast<int>(left.size()) - pos);
        double* channels[2] = {left.data() + pos, right.data() + pos};
        engine.process(channels, 2, count);
    }
}
}

int main()
{
    using Mixorator::DSP::AnalysisEngine;

    // Determinism across common music sample rates. These are not claimed as
    // external BS.1770 certification fixtures; they catch sample-rate and
    // block-boundary regressions in our own K-weighted loudness engine.
    for (double sampleRate : {44100.0, 48000.0, 96000.0})
    {
        AnalysisEngine a;
        AnalysisEngine b;
        a.prepare(sampleRate);
        b.prepare(sampleRate);
        auto leftA = sine(sampleRate, 1000.0, 8.0, 0.1);
        auto rightA = leftA;
        auto leftB = leftA;
        auto rightB = rightA;
        processStereo(a, leftA, rightA, 127);
        processStereo(b, leftB, rightB, 1024);

        const double ia = a.calculateIntegratedLufs();
        const double ib = b.calculateIntegratedLufs();
        if (!std::isfinite(ia) || !std::isfinite(ib)) return fail("Integrated LUFS unavailable for steady sine");
        if (!approx(ia, ib, 1e-9)) return fail("Integrated LUFS depends on host block size");
        if (!std::isfinite(a.shortTermLufs()) || !std::isfinite(a.momentaryLufs()))
            return fail("Momentary/Short-Term loudness unavailable for steady sine");
        if (std::abs(a.calculateLoudnessRangeLu()) > 0.2)
            return fail("Steady sine LRA is not approximately zero");
    }

    // Integrated loudness must ignore a long silent tail through BS.1770
    // gating rather than averaging silence into the programme loudness.
    {
        constexpr double sr = 48000.0;
        AnalysisEngine reference;
        AnalysisEngine withSilence;
        reference.prepare(sr);
        withSilence.prepare(sr);
        auto refL = sine(sr, 1000.0, 8.0, 0.1);
        auto refR = refL;
        processStereo(reference, refL, refR);

        auto fullL = refL;
        auto fullR = refR;
        fullL.resize(static_cast<std::size_t>(sr * 16.0), 0.0);
        fullR.resize(static_cast<std::size_t>(sr * 16.0), 0.0);
        processStereo(withSilence, fullL, fullR);

        const double a = reference.calculateIntegratedLufs();
        const double b = withSilence.calculateIntegratedLufs();
        if (!approx(a, b, 0.15)) return fail("Integrated loudness gating was biased by silent tail");
    }

    // A programme with two sustained level regions must produce non-zero LRA.
    {
        constexpr double sr = 48000.0;
        AnalysisEngine engine;
        engine.prepare(sr);
        auto loud = sine(sr, 1000.0, 10.0, 0.2);
        auto quiet = sine(sr, 1000.0, 10.0, 0.05);
        loud.insert(loud.end(), quiet.begin(), quiet.end());
        auto right = loud;
        processStereo(engine, loud, right);
        const double lra = engine.calculateLoudnessRangeLu();
        if (!std::isfinite(lra) || lra < 8.0 || lra > 14.0)
            return fail("Stepped programme LRA is outside expected broad range");
    }

    std::cout << "All Mixorator loudness metrology tests passed.\n";
    return 0;
}
