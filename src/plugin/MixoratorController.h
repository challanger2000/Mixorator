#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "AnalysisExchange.h"

#include <cstdint>

namespace Mixorator
{
class Controller : public Steinberg::Vst::EditController,
                   public Steinberg::Vst::IDataExchangeReceiver
{
public:
    OBJ_METHODS(Controller, Steinberg::Vst::EditController)
    DEFINE_INTERFACES
        DEF_INTERFACE(Steinberg::Vst::IDataExchangeReceiver)
    END_DEFINE_INTERFACES(Steinberg::Vst::EditController)
    REFCOUNT_METHODS(Steinberg::Vst::EditController)

    static Steinberg::FUnknown* createInstance(void*)
    {
        return static_cast<Steinberg::Vst::IEditController*>(new Controller());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

    void PLUGIN_API queueOpened(
        Steinberg::Vst::DataExchangeUserContextID userContextID,
        Steinberg::uint32 blockSize,
        Steinberg::TBool& dispatchOnBackgroundThread) override;
    void PLUGIN_API queueClosed(
        Steinberg::Vst::DataExchangeUserContextID userContextID) override;
    void PLUGIN_API onDataExchangeBlocksReceived(
        Steinberg::Vst::DataExchangeUserContextID userContextID,
        Steinberg::uint32 numBlocks,
        Steinberg::Vst::DataExchangeBlock* blocks,
        Steinberg::TBool onBackgroundThread) override;

    Steinberg::tresult requestLiveAnalysis() noexcept;
    Steinberg::tresult requestFinalAnalysis() noexcept;

    bool hasAnalysisPacket() const noexcept { return hasPacket_; }
    bool hasDefinitiveFinalSnapshot() const noexcept
    {
        return hasPacket_ && latestPacket_.finalState != 0 &&
               finalSnapshotGeneration_ == latestPacket_.finalizationGeneration;
    }
    const AnalysisExchangePacket& latestAnalysisPacket() const noexcept { return latestPacket_; }

    Analysis::Assessment evaluateLatest(
        Analysis::AnalysisMode mode,
        Analysis::Genre genre,
        Analysis::Era era) const noexcept;

private:
    Steinberg::tresult requestAnalysisState(Steinberg::int64 state) noexcept;
    void requestFinalSnapshot(std::uint64_t generation) noexcept;
    bool consumeFinalSnapshotMessage(Steinberg::Vst::IMessage* message) noexcept;

    Steinberg::Vst::DataExchangeReceiverHandler dataExchange_ {this};
    AnalysisExchangePacket latestPacket_ {};
    bool hasPacket_ {false};
    std::uint64_t requestedFinalGeneration_ {0};
    std::uint64_t finalSnapshotGeneration_ {0};
};
}
