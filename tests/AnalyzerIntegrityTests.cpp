#include "dsp/AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
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
    constexpr double sampleRate = 48000.0;

    // A full-cycle sine has RMS = peak/sqrt(2), therefore conventional
    // peak-to-RMS crest factor is 20*log10(sqrt(2)) = 3.0102999566 dB.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        auto left = sine(sampleRate, 1000.0, 2.0, 0.5);
        auto right = left;
        processStereo(engine, left, right);
        const double expectedRms = 20.0 * std::log10(0.5 / std::sqrt(2.0));
        if (!approx(engine.rmsDbfs(), expectedRms, 0.001)) return fail("Sine RMS calibration failed");
        if (!approx(engine.crestFactorDb(), 3.0102999566, 0.001)) return fail("Sine crest-factor calibration failed");
    }

    // Constant offset should be measured as its arithmetic mean magnitude.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left(48000, 0.01);
        std::vector<double> right(48000, -0.02);
        processStereo(engine, left, right);
        if (!approx(engine.dcOffsetLeftDbfs(), -40.0, 0.001)) return fail("Left DC-offset calibration failed");
        if (!approx(engine.dcOffsetRightDbfs(), 20.0 * std::log10(0.02), 0.001)) return fail("Right DC-offset calibration failed");
    }

    // Raw over-full-scale counting is deliberately strict abs(sample) > 1.0;
    // exactly +/-1.0 must not count.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left {0.0, 1.0, -1.0, 1.0001, -1.2, 0.5};
        std::vector<double> right {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        processStereo(engine, left, right, 6);
        if (engine.clippedSampleCount() != 2) return fail("Over-full-scale sample counting failed");
    }

    // Non-finite input must be counted but must not poison all subsequent
    // analyzer state. Finite peaks/RMS after the block are essential.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left {0.25, std::numeric_limits<double>::quiet_NaN(), 0.5,
                                  std::numeric_limits<double>::infinity(), -0.75};
        std::vector<double> right(left.size(), 0.0);
        processStereo(engine, left, right, static_cast<int>(left.size()));
        if (engine.nonFiniteSampleCount() != 2) return fail("Non-finite sample counting failed");
        if (!std::isfinite(engine.samplePeakDbfs()) || !std::isfinite(engine.rmsDbfs()))
            return fail("Non-finite input poisoned analyzer state");
    }

    // Reset must clear programme state rather than leaking measurements into
    // the next analysis pass.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left(48000, 1.1);
        std::vector<double> right = left;
        processStereo(engine, left, right);
        if (engine.clippedSampleCount() == 0) return fail("Reset precondition failed");
        engine.reset();
        if (engine.clippedSampleCount() != 0 || engine.nonFiniteSampleCount() != 0)
            return fail("Reset did not clear integrity counters");
        if (engine.samplePeakDbfs() > -999.0 || engine.rmsDbfs() > -999.0)
            return fail("Reset did not clear level state");
    }

    std::cout << "All Mixorator analyzer integrity tests passed.\n";
    return 0;
}
