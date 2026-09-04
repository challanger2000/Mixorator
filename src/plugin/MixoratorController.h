#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "AnalysisExchange.h"

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

    bool hasAnalysisPacket() const noexcept { return hasPacket_; }
    const AnalysisExchangePacket& latestAnalysisPacket() const noexcept { return latestPacket_; }

    Analysis::Assessment evaluateLatest(
        Analysis::AnalysisMode mode,
        Analysis::Genre genre,
        Analysis::Era era) const noexcept;

private:
    Steinberg::Vst::DataExchangeReceiverHandler dataExchange_ {this};
    AnalysisExchangePacket latestPacket_ {};
    bool hasPacket_ {false};
};
}
