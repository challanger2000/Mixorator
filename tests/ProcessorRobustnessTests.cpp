#include "plugin/MixoratorProcessor.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
int fail(const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}

Steinberg::Vst::ProcessSetup makeSetup(double sampleRate, Steinberg::int32 sampleSize)
{
    Steinberg::Vst::ProcessSetup setup {};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = sampleSize;
    setup.maxSamplesPerBlock = 4096;
    setup.sampleRate = sampleRate;
    return setup;
}

int processZeroBlock(Mixorator::Processor& processor, Steinberg::int32 sampleSize)
{
    Steinberg::Vst::ProcessData data {};
    data.numSamples = 0;
    data.symbolicSampleSize = sampleSize;
    return processor.process(data) == Steinberg::kResultOk ? 0 : fail("Zero-sample process block failed");
}

template <typename Sample>
int processVariableBlocks(Mixorator::Processor& processor,
                          Steinberg::int32 symbolicSampleSize,
                          double sampleRate)
{
    constexpr std::array<Steinberg::int32, 8> blocks {{1, 3, 17, 64, 127, 511, 1024, 4096}};
    std::uint64_t absoluteSample = 0;

    for (const auto blockSize : blocks)
    {
        std::vector<Sample> inL(static_cast<std::size_t>(blockSize));
        std::vector<Sample> inR(static_cast<std::size_t>(blockSize));
        std::vector<Sample> outL(static_cast<std::size_t>(blockSize), static_cast<Sample>(9));
        std::vector<Sample> outR(static_cast<std::size_t>(blockSize), static_cast<Sample>(-9));

        for (Steinberg::int32 i = 0; i < blockSize; ++i, ++absoluteSample)
        {
            const double phase = 2.0 * 3.14159265358979323846 * 997.0 *
                                 static_cast<double>(absoluteSample) / sampleRate;
            inL[static_cast<std::size_t>(i)] = static_cast<Sample>(0.37 * std::sin(phase));
            inR[static_cast<std::size_t>(i)] = static_cast<Sample>(0.23 * std::cos(phase * 0.73));
        }

        const auto refL = inL;
        const auto refR = inR;

        Steinberg::Vst::AudioBusBuffers input {};
        Steinberg::Vst::AudioBusBuffers output {};
        input.numChannels = output.numChannels = 2;
        input.silenceFlags = 0;

        Steinberg::Vst::ProcessData data {};
        data.numInputs = data.numOutputs = 1;
        data.inputs = &input;
        data.outputs = &output;
        data.numSamples = blockSize;
        data.symbolicSampleSize = symbolicSampleSize;

        if constexpr (sizeof(Sample) == sizeof(Steinberg::Vst::Sample32))
        {
            std::array<Steinberg::Vst::Sample32*, 2> in {{inL.data(), inR.data()}};
            std::array<Steinberg::Vst::Sample32*, 2> out {{outL.data(), outR.data()}};
            input.channelBuffers32 = in.data();
            output.channelBuffers32 = out.data();
            if (processor.process(data) != Steinberg::kResultOk)
                return fail("32-bit variable-block processing failed");
        }
        else
        {
            std::array<Steinberg::Vst::Sample64*, 2> in {{inL.data(), inR.data()}};
            std::array<Steinberg::Vst::Sample64*, 2> out {{outL.data(), outR.data()}};
            input.channelBuffers64 = in.data();
            output.channelBuffers64 = out.data();
            if (processor.process(data) != Steinberg::kResultOk)
                return fail("64-bit variable-block processing failed");
        }

        if (std::memcmp(refL.data(), outL.data(), refL.size() * sizeof(Sample)) != 0 ||
            std::memcmp(refR.data(), outR.data(), refR.size() * sizeof(Sample)) != 0)
            return fail("Variable-block processing changed audio");
    }

    return 0;
}

int testStateTransitions()
{
    Mixorator::Processor processor;
    auto setup = makeSetup(48000.0, Steinberg::Vst::kSample32);
    if (processor.setupProcessing(setup) != Steinberg::kResultOk)
        return fail("Processor setup failed for state-transition test");

    if (processor.analysisState() != Mixorator::Processor::AnalysisState::Live)
        return fail("Processor did not start in LIVE state");
    if (processor.finalizationGeneration() != 0)
        return fail("Initial FINAL generation was not zero");

    processor.requestFinalAnalysis();
    if (processor.analysisState() != Mixorator::Processor::AnalysisState::Live)
        return fail("FINAL request changed state before a process boundary");
    if (processZeroBlock(processor, Steinberg::Vst::kSample32) != 0)
        return 1;
    if (processor.analysisState() != Mixorator::Processor::AnalysisState::Final)
        return fail("FINAL request was not applied at process boundary");
    if (processor.finalizationGeneration() != 1)
        return fail("FINAL generation did not increment exactly once");

    if (processZeroBlock(processor, Steinberg::Vst::kSample32) != 0)
        return 1;
    if (processor.finalizationGeneration() != 1)
        return fail("FINAL generation changed without a new FINAL request");

    processor.requestLiveAnalysis();
    if (processZeroBlock(processor, Steinberg::Vst::kSample32) != 0)
        return 1;
    if (processor.analysisState() != Mixorator::Processor::AnalysisState::Live)
        return fail("LIVE request was not applied at process boundary");

    processor.requestFinalAnalysis();
    if (processZeroBlock(processor, Steinberg::Vst::kSample32) != 0)
        return 1;
    if (processor.analysisState() != Mixorator::Processor::AnalysisState::Final ||
        processor.finalizationGeneration() != 2)
        return fail("Second FINAL transition did not produce generation two");

    return 0;
}
}

int main()
{
    constexpr std::array<double, 4> sampleRates {{44100.0, 48000.0, 96000.0, 192000.0}};

    for (const auto sampleRate : sampleRates)
    {
        {
            Mixorator::Processor processor;
            auto setup = makeSetup(sampleRate, Steinberg::Vst::kSample32);
            if (processor.setupProcessing(setup) != Steinberg::kResultOk)
                return fail("32-bit setupProcessing rejected a supported sample rate");
            if (processVariableBlocks<float>(processor, Steinberg::Vst::kSample32, sampleRate) != 0)
                return 1;
        }

        {
            Mixorator::Processor processor;
            auto setup = makeSetup(sampleRate, Steinberg::Vst::kSample64);
            if (processor.setupProcessing(setup) != Steinberg::kResultOk)
                return fail("64-bit setupProcessing rejected a supported sample rate");
            if (processVariableBlocks<double>(processor, Steinberg::Vst::kSample64, sampleRate) != 0)
                return 1;
        }
    }

    if (testStateTransitions() != 0)
        return 1;

    std::cout << "All Mixorator processor robustness tests passed.\n";
    return 0;
}
