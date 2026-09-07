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

void appendDbfsTone(std::vector<double>& destination, double sampleRate, double seconds, double peakDbfs)
{
    const auto tone = sine(sampleRate, 1000.0, seconds, std::pow(10.0, peakDbfs / 20.0));
    destination.insert(destination.end(), tone.begin(), tone.end());
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

    // EBU Tech 3341 minimum-requirements tests 1 and 2. A stereo 1 kHz sine
    // at -23/-33 dBFS peak must read the same numerical value in LUFS for
    // Momentary, Short-Term and Integrated loudness (accepted tolerance 0.1 LU).
    for (double level : {-23.0, -33.0})
    {
        constexpr double sr = 48000.0;
        AnalysisEngine engine;
        engine.prepare(sr);
        auto left = sine(sr, 1000.0, 20.0, std::pow(10.0, level / 20.0));
        auto right = left;
        processStereo(engine, left, right);
        if (!approx(engine.momentaryLufs(), level, 0.1)) return fail("EBU Tech 3341 Momentary reference failed");
        if (!approx(engine.shortTermLufs(), level, 0.1)) return fail("EBU Tech 3341 Short-Term reference failed");
        if (!approx(engine.calculateIntegratedLufs(), level, 0.1)) return fail("EBU Tech 3341 Integrated steady-tone reference failed");
    }

    // EBU Tech 3341 minimum-requirements tests 3-5 exercise the absolute and
    // relative gates. All three prescribed programmes must integrate to -23 LUFS.
    {
        constexpr double sr = 48000.0;
        const std::vector<std::vector<std::pair<double, double>>> programmes {
            {{10.0, -36.0}, {60.0, -23.0}, {10.0, -36.0}},
            {{10.0, -72.0}, {10.0, -36.0}, {60.0, -23.0}, {10.0, -36.0}, {10.0, -72.0}},
            {{20.0, -26.0}, {20.1, -20.0}, {20.0, -26.0}}
        };
        for (const auto& programme : programmes)
        {
            AnalysisEngine engine;
            engine.prepare(sr);
            std::vector<double> left;
            for (const auto& segment : programme)
                appendDbfsTone(left, sr, segment.first, segment.second);
            auto right = left;
            processStereo(engine, left, right);
            if (!approx(engine.calculateIntegratedLufs(), -23.0, 0.1))
                return fail("EBU Tech 3341 gated Integrated reference failed");
        }
    }

    // Determinism across common music sample rates. These complement the
    // external EBU fixtures above by catching block-boundary regressions.
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
