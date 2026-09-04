#include "MixoratorProcessor.h"
#include "MixoratorIDs.h"
#include "../analysis/AssessmentInput.h"

#include <algorithm>
#include <cstring>

namespace Mixorator
{
Processor::Processor()
{
    setControllerClass(kControllerUID);
}

Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context)
{
    const auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;

    addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

    dataExchange_ = std::make_unique<Steinberg::Vst::DataExchangeHandler>(
        this,
        [](auto& config, const auto&) {
            config.numBlocks = 8;
            config.blockSize = sizeof(AnalysisExchangePacket);
            config.alignment = alignof(AnalysisExchangePacket);
            config.userContextID = kAnalysisExchangeContext;
            return true;
        });

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::terminate()
{
    dataExchange_.reset();
    return AudioEffect::terminate();
}

Steinberg::tresult PLUGIN_API Processor::connect(Steinberg::Vst::IConnectionPoint* other)
{
    const auto result = AudioEffect::connect(other);
    if (dataExchange_)
        dataExchange_->onConnect(other, getHostContext());
    return result;
}

Steinberg::tresult PLUGIN_API Processor::disconnect(Steinberg::Vst::IConnectionPoint* other)
{
    if (dataExchange_)
        dataExchange_->onDisconnect(other);
    return AudioEffect::disconnect(other);
}

Steinberg::tresult PLUGIN_API Processor::setupProcessing(Steinberg::Vst::ProcessSetup& setup)
{
    const auto result = AudioEffect::setupProcessing(setup);
    if (result != Steinberg::kResultOk)
        return result;

    analysis_.prepare(setup.sampleRate);
    exchangeIntervalSamples_ = static_cast<std::uint64_t>(std::max(1.0, setup.sampleRate / 20.0));
    exchangeSampleCounter_ = 0;
    exchangeSequence_ = 0;
    lastPublishedFinalizationGeneration_ = 0;
    exchangeBlock_ = {nullptr, 0, Steinberg::Vst::InvalidDataExchangeBlockID};
    analysisCommand_.store(AnalysisCommand::None, std::memory_order_relaxed);
    analysisState_.store(AnalysisState::Live, std::memory_order_release);
    finalizationGeneration_.store(0, std::memory_order_relaxed);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state)
{
    if (dataExchange_)
    {
        if (state)
            dataExchange_->onActivate(processSetup);
        else
            dataExchange_->onDeactivate();
    }

    if (state)
    {
        analysis_.reset();
        exchangeSampleCounter_ = 0;
        exchangeSequence_ = 0;
        lastPublishedFinalizationGeneration_ = 0;
        analysisCommand_.store(AnalysisCommand::None, std::memory_order_relaxed);
        analysisState_.store(AnalysisState::Live, std::memory_order_release);
        finalizationGeneration_.store(0, std::memory_order_relaxed);
    }

    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs,
    Steinberg::int32 numIns,
    Steinberg::Vst::SpeakerArrangement* outputs,
    Steinberg::int32 numOuts)
{
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == Steinberg::Vst::SpeakerArr::kStereo &&
        outputs[0] == Steinberg::Vst::SpeakerArr::kStereo)
    {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }

    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
{
    return (symbolicSampleSize == Steinberg::Vst::kSample32 ||
            symbolicSampleSize == Steinberg::Vst::kSample64)
               ? Steinberg::kResultTrue
               : Steinberg::kResultFalse;
}

void Processor::requestLiveAnalysis() noexcept
{
    analysisCommand_.store(AnalysisCommand::StartLive, std::memory_order_release);
}

void Processor::requestFinalAnalysis() noexcept
{
    analysisCommand_.store(AnalysisCommand::Finalize, std::memory_order_release);
}

Processor::AnalysisState Processor::analysisState() const noexcept
{
    return analysisState_.load(std::memory_order_acquire);
}

std::uint64_t Processor::finalizationGeneration() const noexcept
{
    return finalizationGeneration_.load(std::memory_order_acquire);
}

DSP::AnalysisSnapshot Processor::captureFinalSnapshot() const noexcept
{
    DSP::AnalysisSnapshot snapshot;

    if (analysisState_.load(std::memory_order_acquire) != AnalysisState::Final)
        return snapshot;

    snapshotReaders_.fetch_add(1, std::memory_order_acq_rel);
    if (analysisState_.load(std::memory_order_acquire) == AnalysisState::Final)
        snapshot = DSP::AnalysisSnapshot::capture(analysis_);
    snapshotReaders_.fetch_sub(1, std::memory_order_release);
    return snapshot;
}

void Processor::handleAnalysisCommandAtBlockBoundary() noexcept
{
    const auto command = analysisCommand_.load(std::memory_order_acquire);

    if (command == AnalysisCommand::Finalize)
    {
        analysisState_.store(AnalysisState::Final, std::memory_order_release);
        finalizationGeneration_.fetch_add(1, std::memory_order_release);

        AnalysisCommand expected = AnalysisCommand::Finalize;
        analysisCommand_.compare_exchange_strong(
            expected, AnalysisCommand::None,
            std::memory_order_acq_rel, std::memory_order_acquire);
        return;
    }

    if (command == AnalysisCommand::StartLive)
    {
        if (snapshotReaders_.load(std::memory_order_acquire) != 0)
            return;

        analysis_.reset();
        analysisState_.store(AnalysisState::Live, std::memory_order_release);
        exchangeSampleCounter_ = exchangeIntervalSamples_;

        AnalysisCommand expected = AnalysisCommand::StartLive;
        analysisCommand_.compare_exchange_strong(
            expected, AnalysisCommand::None,
            std::memory_order_acq_rel, std::memory_order_acquire);
    }
}

void Processor::publishAnalysisExchange(Steinberg::int32 numSamples) noexcept
{
    if (!dataExchange_)
        return;

    exchangeSampleCounter_ += static_cast<std::uint64_t>(std::max<Steinberg::int32>(0, numSamples));
    const auto generation = finalizationGeneration_.load(std::memory_order_acquire);
    const bool finalChanged = generation != lastPublishedFinalizationGeneration_;
    if (exchangeSampleCounter_ < exchangeIntervalSamples_ && !finalChanged)
        return;

    auto block = dataExchange_->getCurrentOrNewBlock();
    if (block.blockID == Steinberg::Vst::InvalidDataExchangeBlockID ||
        block.data == nullptr || block.size < sizeof(AnalysisExchangePacket))
    {
        dataExchange_->discardCurrentBlock();
        return;
    }

    AnalysisExchangePacket packet;
    packet.sequence = ++exchangeSequence_;
    packet.finalizationGeneration = generation;
    packet.finalState = analysisState_.load(std::memory_order_acquire) == AnalysisState::Final ? 1u : 0u;
    packet.metrics = Analysis::AssessmentInput::fromLive(analysis_);

    std::memcpy(block.data, &packet, sizeof(packet));
    if (dataExchange_->sendCurrentBlock())
    {
        exchangeSampleCounter_ = 0;
        lastPublishedFinalizationGeneration_ = generation;
    }
}

Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data)
{
    handleAnalysisCommandAtBlockBoundary();

    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0)
    {
        publishAnalysisExchange(data.numSamples);
        return Steinberg::kResultOk;
    }

    auto& input = data.inputs[0];
    auto& output = data.outputs[0];
    const auto channels = input.numChannels < output.numChannels ? input.numChannels : output.numChannels;
    const bool analyse = analysisState_.load(std::memory_order_acquire) == AnalysisState::Live;

    if (data.symbolicSampleSize == Steinberg::Vst::kSample32)
    {
        if (analyse)
            analysis_.process(input.channelBuffers32, input.numChannels, data.numSamples);

        for (Steinberg::int32 ch = 0; ch < channels; ++ch)
        {
            const auto* in = input.channelBuffers32[ch];
            auto* out = output.channelBuffers32[ch];
            if (!in || !out || in == out)
                continue;

            for (Steinberg::int32 i = 0; i < data.numSamples; ++i)
                out[i] = in[i];
        }
    }
    else if (data.symbolicSampleSize == Steinberg::Vst::kSample64)
    {
        if (analyse)
            analysis_.process(input.channelBuffers64, input.numChannels, data.numSamples);

        for (Steinberg::int32 ch = 0; ch < channels; ++ch)
        {
            const auto* in = input.channelBuffers64[ch];
            auto* out = output.channelBuffers64[ch];
            if (!in || !out || in == out)
                continue;

            for (Steinberg::int32 i = 0; i < data.numSamples; ++i)
                out[i] = in[i];
        }
    }

    output.silenceFlags = input.silenceFlags;
    publishAnalysisExchange(data.numSamples);
    return Steinberg::kResultOk;
}
}
