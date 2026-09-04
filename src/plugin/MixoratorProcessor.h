#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "../dsp/AnalysisEngine.h"
#include "../dsp/AnalysisSnapshot.h"
#include "AnalysisExchange.h"

#include <atomic>
#include <cstdint>
#include <memory>

namespace Mixorator
{
class Processor : public Steinberg::Vst::AudioEffect
{
public:
    enum class AnalysisState : std::uint8_t
    {
        Live,
        Final
    };

    Processor();
    ~Processor() override = default;

    static Steinberg::FUnknown* createInstance(void*)
    {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API connect(Steinberg::Vst::IConnectionPoint* other) override;
    Steinberg::tresult PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint* other) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs,
        Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs,
        Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;

    void requestLiveAnalysis() noexcept;
    void requestFinalAnalysis() noexcept;
    AnalysisState analysisState() const noexcept;
    std::uint64_t finalizationGeneration() const noexcept;

    // Non-realtime only. The mutable programme history is frozen while FINAL is active.
    DSP::AnalysisSnapshot captureFinalSnapshot() const noexcept;

private:
    enum class AnalysisCommand : std::uint8_t
    {
        None,
        StartLive,
        Finalize
    };

    void handleAnalysisCommandAtBlockBoundary() noexcept;
    void publishAnalysisExchange(Steinberg::int32 numSamples) noexcept;

    DSP::AnalysisEngine analysis_;
    std::unique_ptr<Steinberg::Vst::DataExchangeHandler> dataExchange_;
    Steinberg::Vst::DataExchangeBlock exchangeBlock_ {
        nullptr, 0, Steinberg::Vst::InvalidDataExchangeBlockID};
    std::uint64_t exchangeSequence_ {0};
    std::uint64_t lastPublishedFinalizationGeneration_ {0};
    std::uint64_t exchangeSampleCounter_ {0};
    std::uint64_t exchangeIntervalSamples_ {2400};

    std::atomic<AnalysisCommand> analysisCommand_ {AnalysisCommand::None};
    std::atomic<AnalysisState> analysisState_ {AnalysisState::Live};
    std::atomic<std::uint64_t> finalizationGeneration_ {0};
    mutable std::atomic<std::uint32_t> snapshotReaders_ {0};
};
}
