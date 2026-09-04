#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
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
    double rmsDbfs() const noexcept { return rmsDbfs_.load(std::memory_order_relaxed); }
    double crestFactorDb() const noexcept { return crestFactorDb_.load(std::memory_order_relaxed); }
    double momentaryLufs() const noexcept { return momentaryLufs_.load(std::memory_order_relaxed); }
    double shortTermLufs() const noexcept { return shortTermLufs_.load(std::memory_order_relaxed); }
    double lrBalanceDb() const noexcept { return lrBalanceDb_.load(std::memory_order_relaxed); }
    double correlation() const noexcept { return correlation_.load(std::memory_order_relaxed); }
    double stereoWidthDb() const noexcept { return stereoWidthDb_.load(std::memory_order_relaxed); }
    double monoCompatibilityDb() const noexcept { return monoCompatibilityDb_.load(std::memory_order_relaxed); }
    double worstLocalCorrelation() const noexcept { return worstLocalCorrelation_.load(std::memory_order_relaxed); }
    double worstLocalMonoCompatibilityDb() const noexcept { return worstLocalMonoCompatibilityDb_.load(std::memory_order_relaxed); }
    double negativeCorrelationPercent() const noexcept { return negativeCorrelationPercent_.load(std::memory_order_relaxed); }
    double dcOffsetLeftDbfs() const noexcept { return dcOffsetLeftDbfs_.load(std::memory_order_relaxed); }
    double dcOffsetRightDbfs() const noexcept { return dcOffsetRightDbfs_.load(std::memory_order_relaxed); }
    std::uint64_t clippedSampleCount() const noexcept { return clippedSampleCount_.load(std::memory_order_relaxed); }
    std::uint64_t nonFiniteSampleCount() const noexcept { return nonFiniteSampleCount_.load(std::memory_order_relaxed); }
    double lowBandPercent() const noexcept { return lowBandPercent_.load(std::memory_order_relaxed); }
    double lowMidBandPercent() const noexcept { return lowMidBandPercent_.load(std::memory_order_relaxed); }
    double highMidBandPercent() const noexcept { return highMidBandPercent_.load(std::memory_order_relaxed); }
    double highBandPercent() const noexcept { return highBandPercent_.load(std::memory_order_relaxed); }

    double calculateIntegratedLufs() const noexcept;
    double calculatePlrDb() const noexcept;
    double calculateLoudnessRangeLu() const noexcept;

private:
    struct Biquad
    {
        double b0 {1.0}; double b1 {0.0}; double b2 {0.0};
        double a1 {0.0}; double a2 {0.0}; double z1 {0.0}; double z2 {0.0};
        double process(double x) noexcept;
        void clear() noexcept;
    };

    static constexpr std::size_t kFftSize = 1024;

    template <typename Sample>
    void processBlock(Sample* const* channels, int numChannels, int numSamples) noexcept;

    static Biquad makeKWeightingShelf(double sampleRate) noexcept;
    static Biquad makeKWeightingHighPass(double sampleRate) noexcept;
    static double energyToLufs(double meanSquare) noexcept;
    static double linearToDbfs(double value) noexcept;

    void pushWindowSample(std::vector<double>& ring, std::size_t& writeIndex,
                          std::size_t& validSamples, double& sum, double value) noexcept;
    void processTruePeakSample(int channel, double sample) noexcept;
    void updateStereoMetrics() noexcept;
    void finishLocalStereoWindow() noexcept;
    void updateTechnicalMetrics() noexcept;
    void pushTonalSample(double sample, int availableChannels) noexcept;
    void analyseTonalFrame() noexcept;
    void updateTonalMetrics() noexcept;

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

    std::vector<double> loudnessBlocks_;
    std::size_t loudnessBlockCount_ {0};

    std::vector<double> lraShortTermBlocks_;
    std::size_t lraShortTermBlockCount_ {0};
    std::size_t lraHopCounter_ {0};

    Biquad shelf_[2];
    Biquad highPass_[2];

    double truePeakHistory_[2][12] {};
    double truePeakLinear_ {0.0};

    long double rmsSumSquares_ {0.0L};
    std::uint64_t rmsSampleCount_ {0};

    long double leftSumSquares_ {0.0L};
    long double rightSumSquares_ {0.0L};
    long double lrCrossSum_ {0.0L};
    long double midSumSquares_ {0.0L};
    long double sideSumSquares_ {0.0L};
    std::uint64_t stereoSampleCount_ {0};

    // Non-overlapping 100 ms stereo windows expose short phase/mono faults
    // that programme-wide averages can hide.
    long double localLeftSumSquares_ {0.0L};
    long double localRightSumSquares_ {0.0L};
    long double localCrossSum_ {0.0L};
    long double localMidSumSquares_ {0.0L};
    std::size_t localStereoSamples_ {0};
    std::uint64_t localStereoWindowCount_ {0};
    std::uint64_t negativeCorrelationWindowCount_ {0};
    double worstLocalCorrelationRaw_ {1.0};
    double worstLocalMonoCompatibilityDbRaw_ {0.0};

    long double dcSum_[2] {0.0L, 0.0L};
    std::uint64_t dcSampleCount_[2] {0, 0};
    std::uint64_t clippedSampleCountRaw_ {0};
    std::uint64_t nonFiniteSampleCountRaw_ {0};

    double tonalInput_[kFftSize] {};
    double fftReal_[kFftSize] {};
    double fftImag_[kFftSize] {};
    std::size_t tonalWrite_ {0};
    int tonalFrameChannel_ {0};
    long double tonalBandEnergy_[4] {0.0L, 0.0L, 0.0L, 0.0L};

    double samplePeakLinear_ {0.0};
    std::atomic<double> samplePeakDbfs_ {-1000.0};
    std::atomic<double> truePeakDbtp_ {-1000.0};
    std::atomic<double> rmsDbfs_ {-1000.0};
    std::atomic<double> crestFactorDb_ {0.0};
    std::atomic<double> momentaryLufs_ {-1000.0};
    std::atomic<double> shortTermLufs_ {-1000.0};
    std::atomic<double> lrBalanceDb_ {0.0};
    std::atomic<double> correlation_ {1.0};
    std::atomic<double> stereoWidthDb_ {-1000.0};
    std::atomic<double> monoCompatibilityDb_ {0.0};
    std::atomic<double> worstLocalCorrelation_ {1.0};
    std::atomic<double> worstLocalMonoCompatibilityDb_ {0.0};
    std::atomic<double> negativeCorrelationPercent_ {0.0};
    std::atomic<double> dcOffsetLeftDbfs_ {-1000.0};
    std::atomic<double> dcOffsetRightDbfs_ {-1000.0};
    std::atomic<std::uint64_t> clippedSampleCount_ {0};
    std::atomic<std::uint64_t> nonFiniteSampleCount_ {0};
    std::atomic<double> lowBandPercent_ {0.0};
    std::atomic<double> lowMidBandPercent_ {0.0};
    std::atomic<double> highMidBandPercent_ {0.0};
    std::atomic<double> highBandPercent_ {0.0};
};
}
