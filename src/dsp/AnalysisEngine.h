#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace Mixorator::DSP
{
class AnalysisEngine
{
public:
    void prepare(double sampleRate);
    void reset();

    void process(float* const* channels, int numChannels, int numSamples) noexcept;
    void process(double* const* channels, int numChannels, int numSamples) noexcept;

    double samplePeakDbfs() const noexcept { return samplePeakDbfs_.load(std::memory_order_relaxed); }
    double truePeakDbtp() const noexcept { return truePeakDbtp_.load(std::memory_order_relaxed); }
    double momentaryLufs() const noexcept { return momentaryLufs_.load(std::memory_order_relaxed); }
    double shortTermLufs() const noexcept { return shortTermLufs_.load(std::memory_order_relaxed); }

    // Intentionally not called from the audio thread. Integrated loudness requires
    // applying the BS.1770 absolute and relative gates across the stored 400 ms blocks.
    double calculateIntegratedLufs() const noexcept;

private:
    struct Biquad
    {
        double b0 {1.0};
        double b1 {0.0};
        double b2 {0.0};
        double a1 {0.0};
        double a2 {0.0};
        double z1 {0.0};
        double z2 {0.0};

        double process(double x) noexcept;
        void clear() noexcept;
    };

    template <typename Sample>
    void processBlock(Sample* const* channels, int numChannels, int numSamples) noexcept;

    static Biquad makeKWeightingShelf(double sampleRate) noexcept;
    static Biquad makeKWeightingHighPass(double sampleRate) noexcept;
    static double energyToLufs(double meanSquare) noexcept;

    void pushWindowSample(std::vector<double>& ring,
                          std::size_t& writeIndex,
                          std::size_t& validSamples,
                          double& sum,
                          double value) noexcept;
    void processTruePeakSample(int channel, double sample) noexcept;

    double sampleRate_ {48000.0};
    std::size_t momentarySamples_ {0};
    std::size_t shortTermSamples_ {0};
    std::size_t hopSamples_ {0};
    std::size_t samplesSinceBlock_ {0};

    std::vector<double> momentaryRing_;
    std::vector<double> shortTermRing_;
    std::size_t momentaryWrite_ {0};
    std::size_t shortTermWrite_ {0};
    std::size_t momentaryValid_ {0};
    std::size_t shortTermValid_ {0};
    double momentarySum_ {0.0};
    double shortTermSum_ {0.0};

    // Four hours at 100 ms block spacing. Allocated once in prepare(), never on the audio thread.
    std::vector<double> loudnessBlocks_;
    std::size_t loudnessBlockCount_ {0};

    Biquad shelf_[2];
    Biquad highPass_[2];

    // ITU-R BS.1770 Annex 2: 48th-order, 4-phase FIR true-peak interpolator.
    // State is fixed-size so true-peak measurement performs no allocation in process().
    double truePeakHistory_[2][12] {};
    double truePeakLinear_ {0.0};

    double samplePeakLinear_ {0.0};
    std::atomic<double> samplePeakDbfs_ {-1000.0};
    std::atomic<double> truePeakDbtp_ {-1000.0};
    std::atomic<double> momentaryLufs_ {-1000.0};
    std::atomic<double> shortTermLufs_ {-1000.0};
};
}
