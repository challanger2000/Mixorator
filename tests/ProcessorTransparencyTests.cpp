#include "plugin/MixoratorProcessor.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

namespace
{
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

template <typename Sample>
Sample makeSample(std::size_t i, int channel)
{
    const double base = ((static_cast<int>(i % 23) - 11) / 13.0) * (channel == 0 ? 0.91 : -0.73);
    return static_cast<Sample>(base);
}

template <typename Sample>
bool buffersBitIdentical(const std::vector<Sample>& a, const std::vector<Sample>& b)
{
    return a.size() == b.size() &&
           std::memcmp(a.data(), b.data(), a.size() * sizeof(Sample)) == 0;
}

template <typename Sample>
int runOutOfPlaceTest(Steinberg::int32 symbolicSampleSize, Steinberg::int32 numSamples)
{
    Mixorator::Processor processor;

    std::vector<Sample> inL(static_cast<std::size_t>(numSamples));
    std::vector<Sample> inR(static_cast<std::size_t>(numSamples));
    std::vector<Sample> outL(static_cast<std::size_t>(numSamples), static_cast<Sample>(123.0));
    std::vector<Sample> outR(static_cast<std::size_t>(numSamples), static_cast<Sample>(-321.0));

    for (std::size_t i = 0; i < inL.size(); ++i)
    {
        inL[i] = makeSample<Sample>(i, 0);
        inR[i] = makeSample<Sample>(i, 1);
    }

    const auto refL = inL;
    const auto refR = inR;

    Steinberg::Vst::AudioBusBuffers input {};
    Steinberg::Vst::AudioBusBuffers output {};
    input.numChannels = 2;
    output.numChannels = 2;
    input.silenceFlags = 0x2;
    output.silenceFlags = 0;

    if constexpr (std::is_same_v<Sample, float>)
    {
        std::array<Steinberg::Vst::Sample32*, 2> in {{inL.data(), inR.data()}};
        std::array<Steinberg::Vst::Sample32*, 2> out {{outL.data(), outR.data()}};
        input.channelBuffers32 = in.data();
        output.channelBuffers32 = out.data();

        Steinberg::Vst::ProcessData data {};
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &input;
        data.outputs = &output;
        data.numSamples = numSamples;
        data.symbolicSampleSize = symbolicSampleSize;
        if (processor.process(data) != Steinberg::kResultOk)
            return fail("32-bit processor call failed");
    }
    else
    {
        std::array<Steinberg::Vst::Sample64*, 2> in {{inL.data(), inR.data()}};
        std::array<Steinberg::Vst::Sample64*, 2> out {{outL.data(), outR.data()}};
        input.channelBuffers64 = in.data();
        output.channelBuffers64 = out.data();

        Steinberg::Vst::ProcessData data {};
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &input;
        data.outputs = &output;
        data.numSamples = numSamples;
        data.symbolicSampleSize = symbolicSampleSize;
        if (processor.process(data) != Steinberg::kResultOk)
            return fail("64-bit processor call failed");
    }

    if (!buffersBitIdentical(refL, outL) || !buffersBitIdentical(refR, outR))
        return fail("Out-of-place processing changed audio samples");
    if (output.silenceFlags != input.silenceFlags)
        return fail("Silence flags were not preserved");

    return 0;
}

template <typename Sample>
int runInPlaceTest(Steinberg::int32 symbolicSampleSize, Steinberg::int32 numSamples)
{
    Mixorator::Processor processor;

    std::vector<Sample> left(static_cast<std::size_t>(numSamples));
    std::vector<Sample> right(static_cast<std::size_t>(numSamples));
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        left[i] = makeSample<Sample>(i, 0);
        right[i] = makeSample<Sample>(i, 1);
    }

    const auto refL = left;
    const auto refR = right;

    Steinberg::Vst::AudioBusBuffers input {};
    Steinberg::Vst::AudioBusBuffers output {};
    input.numChannels = output.numChannels = 2;
    input.silenceFlags = 0x1;

    if constexpr (std::is_same_v<Sample, float>)
    {
        std::array<Steinberg::Vst::Sample32*, 2> channels {{left.data(), right.data()}};
        input.channelBuffers32 = channels.data();
        output.channelBuffers32 = channels.data();

        Steinberg::Vst::ProcessData data {};
        data.numInputs = data.numOutputs = 1;
        data.inputs = &input;
        data.outputs = &output;
        data.numSamples = numSamples;
        data.symbolicSampleSize = symbolicSampleSize;
        if (processor.process(data) != Steinberg::kResultOk)
            return fail("32-bit in-place processor call failed");
    }
    else
    {
        std::array<Steinberg::Vst::Sample64*, 2> channels {{left.data(), right.data()}};
        input.channelBuffers64 = channels.data();
        output.channelBuffers64 = channels.data();

        Steinberg::Vst::ProcessData data {};
        data.numInputs = data.numOutputs = 1;
        data.inputs = &input;
        data.outputs = &output;
        data.numSamples = numSamples;
        data.symbolicSampleSize = symbolicSampleSize;
        if (processor.process(data) != Steinberg::kResultOk)
            return fail("64-bit in-place processor call failed");
    }

    if (!buffersBitIdentical(refL, left) || !buffersBitIdentical(refR, right))
        return fail("In-place processing changed audio samples");
    if (output.silenceFlags != input.silenceFlags)
        return fail("In-place silence flags were not preserved");

    return 0;
}
}

int main()
{
    constexpr std::array<Steinberg::int32, 5> blockSizes {{1, 17, 127, 512, 2048}};

    for (const auto blockSize : blockSizes)
    {
        if (runOutOfPlaceTest<float>(Steinberg::Vst::kSample32, blockSize) != 0)
            return 1;
        if (runInPlaceTest<float>(Steinberg::Vst::kSample32, blockSize) != 0)
            return 1;
        if (runOutOfPlaceTest<double>(Steinberg::Vst::kSample64, blockSize) != 0)
            return 1;
        if (runInPlaceTest<double>(Steinberg::Vst::kSample64, blockSize) != 0)
            return 1;
    }

    std::cout << "All Mixorator processor transparency tests passed.\n";
    return 0;
}
