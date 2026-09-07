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

std::vector<double> sine(double sampleRate, double frequency, double seconds,
                         double amplitude, double phase)
{
    std::vector<double> result(static_cast<std::size_t>(std::llround(sampleRate * seconds)));
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = amplitude * std::sin(2.0 * kPi * frequency * static_cast<double>(i) / sampleRate + phase);
    return result;
}

std::vector<double> taperedSine(double sampleRate, double frequency, double seconds,
                                double amplitude, double phase)
{
    auto result = sine(sampleRate, frequency, seconds, amplitude, phase);
    const std::size_t fade = static_cast<std::size_t>(std::llround(sampleRate * 0.010));
    for (std::size_t i = 0; i < std::min(fade, result.size() / 2); ++i)
    {
        const double gain = static_cast<double>(i) / static_cast<double>(fade);
        result[i] *= gain;
        result[result.size() - 1 - i] *= gain;
    }
    return result;
}

void processStereo(Mixorator::DSP::AnalysisEngine& engine,
                   std::vector<double>& left,
                   std::vector<double>& right,
                   int blockSize)
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

    // EBU Tech 3341 minimum-requirements true-peak tests 15-19. The prescribed
    // tones verify the ITU-R BS.1770 4x interpolating meter at difficult phases
    // and normalized frequencies. EBU tolerance is +0.2/-0.4 dBTP.
    struct EbuTruePeakCase { double frequencyRatio; double amplitude; double phaseDegrees; double expectedDbtp; };
    const EbuTruePeakCase ebuCases[] {
        {1.0 / 4.0, 0.50,  0.0, -6.0},
        {1.0 / 4.0, 0.50, 45.0, -6.0},
        {1.0 / 6.0, 0.50, 60.0, -6.0},
        {1.0 / 8.0, 0.50, 67.5, -6.0},
        {1.0 / 4.0, 1.41, 45.0,  3.0}
    };
    for (const auto& test : ebuCases)
    {
        constexpr double sampleRate = 48000.0;
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        auto left = taperedSine(sampleRate, sampleRate * test.frequencyRatio, 2.0,
                                test.amplitude, test.phaseDegrees * kPi / 180.0);
        auto right = left;
        processStereo(engine, left, right, 257);
        const double measured = engine.truePeakDbtp();
        if (measured < test.expectedDbtp - 0.4 || measured > test.expectedDbtp + 0.2)
            return fail("EBU Tech 3341 True Peak reference failed");
    }

    // Fundamental invariant: true peak must never be lower than sample peak.
    for (double sampleRate : {44100.0, 48000.0, 96000.0})
    {
        for (double frequency : {997.0, 0.25 * sampleRate, 0.40 * sampleRate})
        {
            AnalysisEngine engine;
            engine.prepare(sampleRate);
            auto left = sine(sampleRate, frequency, 2.0, 0.95, 0.37);
            auto right = left;
            processStereo(engine, left, right, 257);
            if (engine.truePeakDbtp() + 1e-9 < engine.samplePeakDbfs())
                return fail("True Peak fell below Sample Peak");
        }
    }

    // For a steady band-limited sine the continuous-time peak is simply the
    // sine amplitude. At moderate normalized frequencies the interpolation
    // estimate should stay close to that analytical reference.
    for (double sampleRate : {44100.0, 48000.0, 96000.0})
    {
        const double amplitude = 0.95;
        const double expectedDb = 20.0 * std::log10(amplitude);
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        auto left = sine(sampleRate, 0.25 * sampleRate, 3.0, amplitude, 0.37);
        auto right = left;
        processStereo(engine, left, right, 311);
        if (!approx(engine.truePeakDbtp(), expectedDb, 0.20))
            return fail("True Peak sine calibration exceeded 0.20 dB tolerance");
    }

    // The meter must expose an inter-sample peak when sample positions miss
    // the continuous waveform maximum.
    {
        constexpr double sampleRate = 48000.0;
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        auto left = sine(sampleRate, sampleRate * 0.25, 3.0, 0.95, 0.37);
        auto right = left;
        processStereo(engine, left, right, 193);
        if (engine.truePeakDbtp() < engine.samplePeakDbfs() + 0.30)
            return fail("Inter-sample peak was not detected");
    }

    // Host block partitioning must not change a programme true-peak result.
    {
        constexpr double sampleRate = 48000.0;
        auto source = sine(sampleRate, 12000.0, 3.0, 0.95, 0.37);
        auto leftA = source; auto rightA = source;
        auto leftB = source; auto rightB = source;
        AnalysisEngine a; AnalysisEngine b;
        a.prepare(sampleRate); b.prepare(sampleRate);
        processStereo(a, leftA, rightA, 17);
        processStereo(b, leftB, rightB, 2048);
        if (!approx(a.truePeakDbtp(), b.truePeakDbtp(), 1e-12))
            return fail("True Peak depends on host block size");
    }

    // Reset must clear FIR history and peak state, otherwise a previous loud
    // programme could leak into the next analysis pass.
    {
        constexpr double sampleRate = 48000.0;
        AnalysisEngine engine;
        engine.prepare(sampleRate);
        auto loud = sine(sampleRate, 12000.0, 1.0, 0.99, 0.37);
        auto right = loud;
        processStereo(engine, loud, right, 127);
        engine.reset();
        std::vector<double> silence(4096, 0.0);
        auto silenceRight = silence;
        processStereo(engine, silence, silenceRight, 127);
        if (engine.truePeakDbtp() > -999.0 || engine.samplePeakDbfs() > -999.0)
            return fail("Reset leaked previous peak state");
    }

    std::cout << "All Mixorator true-peak metrology tests passed.\n";
    return 0;
}
