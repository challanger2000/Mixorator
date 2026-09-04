#include "AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Mixorator::DSP
{
namespace
{
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kLoudnessOffset = -0.691;
constexpr double kAbsoluteGateLufs = -70.0;
constexpr double kRelativeGateLu = 10.0;
constexpr double kMinimumEnergy = 1.0e-20;

// ITU-R BS.1770 Annex 2 example coefficients for a 48th-order,
// 4-phase FIR interpolator. Floating-point processing needs no initial 12.04 dB attenuation.
constexpr double kTruePeakFir[12][4] = {
    { 0.0017089843750, -0.0291748046875, -0.0189208984375, -0.0083007812500 },
    { 0.0109863281250,  0.0292968750000,  0.0330810546875,  0.0148925781250 },
    {-0.0196533203125, -0.0517578125000, -0.0582275390625, -0.0266113281250 },
    { 0.0332031250000,  0.0891113281250,  0.1015625000000,  0.0476074218750 },
    {-0.0594482421875, -0.1665039062500, -0.2003173828125, -0.1022949218750 },
    { 0.1373291015625,  0.4650878906250,  0.7797851562500,  0.9721679687500 },
    { 0.9721679687500,  0.7797851562500,  0.4650878906250,  0.1373291015625 },
    {-0.1022949218750, -0.2003173828125, -0.1665039062500, -0.0594482421875 },
    { 0.0476074218750,  0.1015625000000,  0.0891113281250,  0.0332031250000 },
    {-0.0266113281250, -0.0582275390625, -0.0517578125000, -0.0196533203125 },
    { 0.0148925781250,  0.0330810546875,  0.0292968750000,  0.0109863281250 },
    {-0.0083007812500, -0.0189208984375, -0.0291748046875,  0.0017089843750 }
};
}

double AnalysisEngine::Biquad::process(double x) noexcept
{
    const double y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

void AnalysisEngine::Biquad::clear() noexcept
{
    z1 = 0.0;
    z2 = 0.0;
}

AnalysisEngine::Biquad AnalysisEngine::makeKWeightingShelf(double sampleRate) noexcept
{
    constexpr double gainDb = 3.999843853973347;
    constexpr double f0 = 1681.974450955533;
    constexpr double q = 0.7071752369554196;
    constexpr double vbExponent = 0.4996667741545416;

    const double k = std::tan(kPi * f0 / sampleRate);
    const double vh = std::pow(10.0, gainDb / 20.0);
    const double vb = std::pow(vh, vbExponent);
    const double a0 = 1.0 + k / q + k * k;

    Biquad filter;
    filter.b0 = (vh + vb * k / q + k * k) / a0;
    filter.b1 = 2.0 * (k * k - vh) / a0;
    filter.b2 = (vh - vb * k / q + k * k) / a0;
    filter.a1 = 2.0 * (k * k - 1.0) / a0;
    filter.a2 = (1.0 - k / q + k * k) / a0;
    return filter;
}

AnalysisEngine::Biquad AnalysisEngine::makeKWeightingHighPass(double sampleRate) noexcept
{
    constexpr double f0 = 38.13547087602444;
    constexpr double q = 0.5003270373238773;

    const double k = std::tan(kPi * f0 / sampleRate);
    const double a0 = 1.0 + k / q + k * k;

    Biquad filter;
    filter.b0 = 1.0;
    filter.b1 = -2.0;
    filter.b2 = 1.0;
    filter.a1 = 2.0 * (k * k - 1.0) / a0;
    filter.a2 = (1.0 - k / q + k * k) / a0;
    return filter;
}

double AnalysisEngine::energyToLufs(double meanSquare) noexcept
{
    if (meanSquare <= kMinimumEnergy)
        return -std::numeric_limits<double>::infinity();

    return kLoudnessOffset + 10.0 * std::log10(meanSquare);
}

void AnalysisEngine::prepare(double sampleRate)
{
    sampleRate_ = sampleRate > 1.0 ? sampleRate : 48000.0;
    momentarySamples_ = std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(sampleRate_ * 0.400)));
    shortTermSamples_ = std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(sampleRate_ * 3.000)));
    hopSamples_ = std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(sampleRate_ * 0.100)));

    momentaryRing_.assign(momentarySamples_, 0.0);
    shortTermRing_.assign(shortTermSamples_, 0.0);

    constexpr std::size_t maxBlocksForFourHours = 4u * 60u * 60u * 10u;
    loudnessBlocks_.assign(maxBlocksForFourHours, 0.0);

    shelf_[0] = makeKWeightingShelf(sampleRate_);
    shelf_[1] = shelf_[0];
    highPass_[0] = makeKWeightingHighPass(sampleRate_);
    highPass_[1] = highPass_[0];

    reset();
}

void AnalysisEngine::reset()
{
    std::fill(momentaryRing_.begin(), momentaryRing_.end(), 0.0);
    std::fill(shortTermRing_.begin(), shortTermRing_.end(), 0.0);

    momentaryWrite_ = 0;
    shortTermWrite_ = 0;
    momentaryValid_ = 0;
    shortTermValid_ = 0;
    momentarySum_ = 0.0;
    shortTermSum_ = 0.0;
    samplesSinceBlock_ = 0;
    loudnessBlockCount_ = 0;
    samplePeakLinear_ = 0.0;
    truePeakLinear_ = 0.0;

    for (auto& filter : shelf_)
        filter.clear();
    for (auto& filter : highPass_)
        filter.clear();
    for (auto& channelHistory : truePeakHistory_)
        std::fill(channelHistory, channelHistory + 12, 0.0);

    samplePeakDbfs_.store(-1000.0, std::memory_order_relaxed);
    truePeakDbtp_.store(-1000.0, std::memory_order_relaxed);
    momentaryLufs_.store(-1000.0, std::memory_order_relaxed);
    shortTermLufs_.store(-1000.0, std::memory_order_relaxed);
}

void AnalysisEngine::pushWindowSample(std::vector<double>& ring,
                                      std::size_t& writeIndex,
                                      std::size_t& validSamples,
                                      double& sum,
                                      double value) noexcept
{
    if (ring.empty())
        return;

    if (validSamples == ring.size())
        sum -= ring[writeIndex];
    else
        ++validSamples;

    ring[writeIndex] = value;
    sum += value;
    writeIndex = (writeIndex + 1) % ring.size();
}

void AnalysisEngine::processTruePeakSample(int channel, double sample) noexcept
{
    auto& history = truePeakHistory_[channel];
    for (int i = 11; i > 0; --i)
        history[i] = history[i - 1];
    history[0] = sample;

    // By definition the reconstructed waveform includes the original sample positions.
    truePeakLinear_ = std::max(truePeakLinear_, std::abs(sample));

    for (int phase = 0; phase < 4; ++phase)
    {
        double interpolated = 0.0;
        for (int tap = 0; tap < 12; ++tap)
            interpolated += history[tap] * kTruePeakFir[tap][phase];

        truePeakLinear_ = std::max(truePeakLinear_, std::abs(interpolated));
    }
}

template <typename Sample>
void AnalysisEngine::processBlock(Sample* const* channels, int numChannels, int numSamples) noexcept
{
    if (!channels || numChannels <= 0 || numSamples <= 0 || momentaryRing_.empty() || shortTermRing_.empty())
        return;

    const int channelsToMeasure = std::min(numChannels, 2);

    for (int i = 0; i < numSamples; ++i)
    {
        double weightedEnergy = 0.0;

        for (int ch = 0; ch < channelsToMeasure; ++ch)
        {
            if (!channels[ch])
                continue;

            const double sample = static_cast<double>(channels[ch][i]);
            samplePeakLinear_ = std::max(samplePeakLinear_, std::abs(sample));
            processTruePeakSample(ch, sample);

            double filtered = shelf_[ch].process(sample);
            filtered = highPass_[ch].process(filtered);
            weightedEnergy += filtered * filtered;
        }

        pushWindowSample(momentaryRing_, momentaryWrite_, momentaryValid_, momentarySum_, weightedEnergy);
        pushWindowSample(shortTermRing_, shortTermWrite_, shortTermValid_, shortTermSum_, weightedEnergy);

        ++samplesSinceBlock_;
        if (samplesSinceBlock_ >= hopSamples_)
        {
            samplesSinceBlock_ = 0;

            if (momentaryValid_ == momentarySamples_)
            {
                const double meanSquare = momentarySum_ / static_cast<double>(momentarySamples_);
                momentaryLufs_.store(energyToLufs(meanSquare), std::memory_order_relaxed);

                if (loudnessBlockCount_ < loudnessBlocks_.size())
                    loudnessBlocks_[loudnessBlockCount_++] = meanSquare;
            }

            if (shortTermValid_ == shortTermSamples_)
            {
                const double meanSquare = shortTermSum_ / static_cast<double>(shortTermSamples_);
                shortTermLufs_.store(energyToLufs(meanSquare), std::memory_order_relaxed);
            }
        }
    }

    const double peakDb = samplePeakLinear_ > 0.0
                              ? 20.0 * std::log10(samplePeakLinear_)
                              : -std::numeric_limits<double>::infinity();
    const double truePeakDb = truePeakLinear_ > 0.0
                                  ? 20.0 * std::log10(truePeakLinear_)
                                  : -std::numeric_limits<double>::infinity();

    samplePeakDbfs_.store(peakDb, std::memory_order_relaxed);
    truePeakDbtp_.store(truePeakDb, std::memory_order_relaxed);
}

void AnalysisEngine::process(float* const* channels, int numChannels, int numSamples) noexcept
{
    processBlock(channels, numChannels, numSamples);
}

void AnalysisEngine::process(double* const* channels, int numChannels, int numSamples) noexcept
{
    processBlock(channels, numChannels, numSamples);
}

double AnalysisEngine::calculateIntegratedLufs() const noexcept
{
    if (loudnessBlockCount_ == 0)
        return -std::numeric_limits<double>::infinity();

    const double absoluteGateEnergy = std::pow(10.0, (kAbsoluteGateLufs - kLoudnessOffset) / 10.0);

    double absoluteGatedSum = 0.0;
    std::size_t absoluteGatedCount = 0;

    for (std::size_t i = 0; i < loudnessBlockCount_; ++i)
    {
        const double energy = loudnessBlocks_[i];
        if (energy >= absoluteGateEnergy)
        {
            absoluteGatedSum += energy;
            ++absoluteGatedCount;
        }
    }

    if (absoluteGatedCount == 0)
        return -std::numeric_limits<double>::infinity();

    const double absoluteGatedMean = absoluteGatedSum / static_cast<double>(absoluteGatedCount);
    const double relativeGateLufs = energyToLufs(absoluteGatedMean) - kRelativeGateLu;
    const double relativeGateEnergy = std::pow(10.0, (relativeGateLufs - kLoudnessOffset) / 10.0);
    const double finalGateEnergy = std::max(absoluteGateEnergy, relativeGateEnergy);

    double finalSum = 0.0;
    std::size_t finalCount = 0;

    for (std::size_t i = 0; i < loudnessBlockCount_; ++i)
    {
        const double energy = loudnessBlocks_[i];
        if (energy >= finalGateEnergy)
        {
            finalSum += energy;
            ++finalCount;
        }
    }

    return finalCount > 0
               ? energyToLufs(finalSum / static_cast<double>(finalCount))
               : -std::numeric_limits<double>::infinity();
}
}
