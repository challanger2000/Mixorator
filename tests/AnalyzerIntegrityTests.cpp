#include "dsp/AnalysisEngine.h"

#include <algorithm>
#include <array>
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

std::array<double, 8> detailedTonal(const Mixorator::DSP::AnalysisEngine& engine)
{
    return {engine.subBandPercent(), engine.bassBandPercent(),
            engine.lowMidBodyBandPercent(), engine.midBandPercent(),
            engine.presenceBandPercent(), engine.upperPresenceBandPercent(),
            engine.brillianceBandPercent(), engine.airBandPercent()};
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

    // Each detailed tonal band must respond predominantly to a tone placed well
    // inside that band. Frequencies are deliberately kept away from boundaries
    // so Hann-window leakage and FFT-bin spacing cannot make the test brittle.
    {
        constexpr std::array<double, 8> frequencies {46.875, 140.625, 375.0, 984.375,
                                                      3000.0, 6000.0, 10000.0, 15000.0};
        for (std::size_t expected = 0; expected < frequencies.size(); ++expected)
        {
            AnalysisEngine engine;
            engine.prepare(sampleRate);
            auto left = sine(sampleRate, frequencies[expected], 1.0, 0.25);
            auto right = left;
            processStereo(engine, left, right, 257);
            const auto bands = detailedTonal(engine);
            const auto dominant = static_cast<std::size_t>(std::distance(
                bands.begin(), std::max_element(bands.begin(), bands.end())));
            if (dominant != expected || bands[expected] < 90.0)
                return fail("Detailed tonal band frequency classification failed");
        }
    }

    // The legacy four-band outputs must remain exact sums of adjacent detailed
    // bands. This protects all existing genre/style scoring while the finer
    // measurements are still measurement-only.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        auto left = sine(sampleRate, 140.625, 1.0, 0.20);
        auto component = sine(sampleRate, 375.0, 1.0, 0.16);
        auto presence = sine(sampleRate, 3000.0, 1.0, 0.12);
        auto air = sine(sampleRate, 15000.0, 1.0, 0.08);
        for (std::size_t i = 0; i < left.size(); ++i)
            left[i] += component[i] + presence[i] + air[i];
        auto right = left;
        processStereo(engine, left, right, 257);
        const auto b = detailedTonal(engine);
        if (!approx(engine.lowBandPercent(), b[0] + b[1], 1e-9) ||
            !approx(engine.lowMidBandPercent(), b[2] + b[3], 1e-9) ||
            !approx(engine.highMidBandPercent(), b[4] + b[5], 1e-9) ||
            !approx(engine.highBandPercent(), b[6] + b[7], 1e-9))
            return fail("Legacy tonal compatibility aggregation failed");
        double total = 0.0;
        for (double value : b) total += value;
        if (!approx(total, 100.0, 1e-9)) return fail("Detailed tonal percentages do not normalize to 100 percent");
    }

    // Floating-point samples above full scale can occur after lossy decoding
    // or upstream gain and are not, by themselves, proof of hard clipping.
    // Isolated legal +/-1.0 samples must not be falsely counted either.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left {0.0, 1.0, -1.0, 1.0001, -1.2, 0.5};
        std::vector<double> right {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        processStereo(engine, left, right, 6);
        if (engine.clippedSampleCount() != 0) return fail("Floating overs were falsely counted as hard clipping");
        if (engine.truePeakDbtp() <= 0.0) return fail("Floating overs were not retained as a positive peak event");
    }

    // Two consecutive exact full-scale samples are still not enough evidence
    // for clipping; this avoids declaring an ordinary legal peak as damaged.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left {0.2, 1.0, 1.0, 0.2, -1.0, -1.0, 0.2};
        std::vector<double> right(left.size(), 0.0);
        processStereo(engine, left, right, 2);
        if (engine.clippedSampleCount() != 0) return fail("Short legal full-scale peak was falsely counted as clipping");
    }

    // Three or more consecutive same-polarity samples exactly at full scale
    // are a conservative digital flat-top signature. The run must be detected
    // even when it crosses host process-block boundaries.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left {0.25, 1.0, 1.0, 1.0, 1.0, 0.25,
                                  -1.0, -1.0, -1.0, 0.25};
        std::vector<double> right(left.size(), 0.0);
        processStereo(engine, left, right, 2);
        if (engine.clippedSampleCount() != 7) return fail("Full-scale flat-top clipping detection failed");
    }

    // Alternating full-scale polarity is not a flat top. A legitimate digital
    // square-like signal at +/-1.0 must therefore not trigger this detector.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left {1.0, -1.0, 1.0, -1.0, 1.0, -1.0};
        std::vector<double> right(left.size(), 0.0);
        processStereo(engine, left, right, 1);
        if (engine.clippedSampleCount() != 0) return fail("Alternating full-scale signal was falsely counted as clipping");
    }

    // A deliberately hard-clipped sine should produce obvious exact full-scale
    // plateaus while an unclipped 0 dBFS sine should remain legal.
    {
        AnalysisEngine legal;
        legal.prepare(sampleRate);
        auto legalLeft = sine(sampleRate, 1000.0, 0.1, 1.0);
        auto legalRight = legalLeft;
        processStereo(legal, legalLeft, legalRight, 127);
        if (legal.clippedSampleCount() != 0) return fail("Legal 0 dBFS sine was falsely counted as clipping");

        AnalysisEngine clipped;
        clipped.prepare(sampleRate);
        auto clippedLeft = sine(sampleRate, 1000.0, 0.1, 1.2);
        for (auto& sample : clippedLeft)
            sample = std::max(-1.0, std::min(1.0, sample));
        auto clippedRight = clippedLeft;
        processStereo(clipped, clippedLeft, clippedRight, 127);
        if (clipped.clippedSampleCount() == 0) return fail("Hard-clipped 0 dBFS sine was not detected");
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
    // the next analysis pass, including a partially accumulated flat-top run.
    {
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        std::vector<double> left(48000, 1.0);
        std::vector<double> right = left;
        processStereo(engine, left, right);
        if (engine.clippedSampleCount() == 0) return fail("Reset precondition failed");
        engine.reset();
        if (engine.clippedSampleCount() != 0 || engine.nonFiniteSampleCount() != 0)
            return fail("Reset did not clear integrity counters");
        if (engine.samplePeakDbfs() > -999.0 || engine.rmsDbfs() > -999.0)
            return fail("Reset did not clear level state");

        std::vector<double> pendingLeft {1.0, 1.0};
        std::vector<double> pendingRight(pendingLeft.size(), 0.0);
        processStereo(engine, pendingLeft, pendingRight, 2);
        engine.reset();
        std::vector<double> afterResetLeft {1.0, 0.0};
        std::vector<double> afterResetRight(afterResetLeft.size(), 0.0);
        processStereo(engine, afterResetLeft, afterResetRight, 2);
        if (engine.clippedSampleCount() != 0) return fail("Reset leaked a pending full-scale clipping run");
    }

    // The realtime reset intentionally leaves large ring/FFT payloads in memory.
    // Their old contents must be unreachable: after reset, processing the same
    // programme must match a freshly prepared engine.
    {
        AnalysisEngine reused;
        AnalysisEngine fresh;
        reused.prepare(sampleRate);
        fresh.prepare(sampleRate);

        auto oldLeft = sine(sampleRate, 100.0, 4.0, 0.9);
        auto oldRight = oldLeft;
        processStereo(reused, oldLeft, oldRight, 127);
        reused.reset();

        auto newLeftA = sine(sampleRate, 4000.0, 4.0, 0.05);
        auto newRightA = newLeftA;
        auto newLeftB = newLeftA;
        auto newRightB = newRightA;
        processStereo(reused, newLeftA, newRightA, 127);
        processStereo(fresh, newLeftB, newRightB, 127);

        if (!approx(reused.calculateIntegratedLufs(), fresh.calculateIntegratedLufs(), 1e-12) ||
            !approx(reused.momentaryLufs(), fresh.momentaryLufs(), 1e-12) ||
            !approx(reused.shortTermLufs(), fresh.shortTermLufs(), 1e-12) ||
            !approx(reused.truePeakDbtp(), fresh.truePeakDbtp(), 1e-12))
            return fail("Reset leaked stale loudness/peak history into the next analysis");
        if (!approx(reused.lowBandPercent(), fresh.lowBandPercent(), 1e-12) ||
            !approx(reused.lowMidBandPercent(), fresh.lowMidBandPercent(), 1e-12) ||
            !approx(reused.highMidBandPercent(), fresh.highMidBandPercent(), 1e-12) ||
            !approx(reused.highBandPercent(), fresh.highBandPercent(), 1e-12))
            return fail("Reset leaked stale tonal history into the next analysis");
        const auto reusedDetailed = detailedTonal(reused);
        const auto freshDetailed = detailedTonal(fresh);
        for (std::size_t i = 0; i < reusedDetailed.size(); ++i)
            if (!approx(reusedDetailed[i], freshDetailed[i], 1e-12))
                return fail("Reset leaked stale detailed tonal history into the next analysis");
    }

    std::cout << "All Mixorator analyzer integrity tests passed.\n";
    return 0;
}
