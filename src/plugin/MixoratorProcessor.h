#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "../dsp/AnalysisEngine.h"
#include "../dsp/AnalysisSnapshot.h"

#include <atomic>
#include <cstdint>

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
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs,
        Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs,
        Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;

    // These requests are lock-free. They are applied by the audio thread at a
    // process-block boundary so AnalysisEngine's non-atomic history never races
    // with reset/final snapshot reads.
    void requestLiveAnalysis() noexcept;
    void requestFinalAnalysis() noexcept;
    AnalysisState analysisState() const noexcept;
    std::uint64_t finalizationGeneration() const noexcept;

    // Call only from a non-realtime thread. Returns valid=false until FINAL has
    // been acknowledged by the audio thread. A concurrent LIVE request is
    // deferred while this immutable snapshot is being copied/calculated.
    DSP::AnalysisSnapshot captureFinalSnapshot() const noexcept;

private:
    enum class AnalysisCommand : std::uint8_t
    {
        None,
        StartLive,
        Finalize
    };

    void handleAnalysisCommandAtBlockBoundary() noexcept;

    DSP::AnalysisEngine analysis_;
    std::atomic<AnalysisCommand> analysisCommand_ {AnalysisCommand::None};
    std::atomic<AnalysisState> analysisState_ {AnalysisState::Live};
    std::atomic<std::uint64_t> finalizationGeneration_ {0};
    mutable std::atomic<std::uint32_t> snapshotReaders_ {0};
};
}
