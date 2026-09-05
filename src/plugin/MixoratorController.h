#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include "vstgui/uidescription/uiattributes.h"
#include "AnalysisExchange.h"

#include <cstdint>

namespace VSTGUI
{
class CControl;
class COptionMenu;
class CTextLabel;
class CView;
}

namespace Mixorator
{
class Controller : public Steinberg::Vst::EditController,
                   public Steinberg::Vst::IDataExchangeReceiver,
                   public VSTGUI::VST3EditorDelegate,
                   public VSTGUI::IControlListener
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
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;

    void PLUGIN_API queueOpened(Steinberg::Vst::DataExchangeUserContextID userContextID,
                                Steinberg::uint32 blockSize,
                                Steinberg::TBool& dispatchOnBackgroundThread) override;
    void PLUGIN_API queueClosed(Steinberg::Vst::DataExchangeUserContextID userContextID) override;
    void PLUGIN_API onDataExchangeBlocksReceived(Steinberg::Vst::DataExchangeUserContextID userContextID,
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

    Analysis::Assessment evaluateLatest(Analysis::AnalysisMode mode,
                                        Analysis::Genre genre,
                                        Analysis::Era era) const noexcept;

    VSTGUI::CView* verifyView(VSTGUI::CView* view,
                              const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description,
                              VSTGUI::VST3Editor* editor) override;
    void didOpen(VSTGUI::VST3Editor* editor) override;
    void willClose(VSTGUI::VST3Editor* editor) override;
    void valueChanged(VSTGUI::CControl* control) override;

private:
    enum UiTag : std::int32_t
    {
        kUiMix = 10001,
        kUiMaster = 10002,
        kUiLive = 10003,
        kUiFinal = 10004,
        kUiGenre = 10005,
        kUiEra = 10006,
        kUiReset = 10007,
        kUiAnalyze = 10008,
        kUiDetails = 10009,
        kUiBack = 10010
    };

    Steinberg::tresult requestAnalysisState(Steinberg::int64 state) noexcept;
    void requestFinalSnapshot(std::uint64_t generation) noexcept;
    bool consumeFinalSnapshotMessage(Steinberg::Vst::IMessage* message) noexcept;

    void refreshUi() noexcept;
    void clearUiPointers() noexcept;
    void bindNamedView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes) noexcept;
    void updateSelectionControls() noexcept;
    void updatePageVisibility() noexcept;

    Steinberg::Vst::DataExchangeReceiverHandler dataExchange_ {this};
    AnalysisExchangePacket latestPacket_ {};
    bool hasPacket_ {false};
    std::uint64_t requestedFinalGeneration_ {0};
    std::uint64_t finalSnapshotGeneration_ {0};

    Analysis::AnalysisMode uiMode_ {Analysis::AnalysisMode::Mix};
    Analysis::Genre uiGenre_ {Analysis::Genre::General};
    Analysis::Era uiEra_ {Analysis::Era::Modern};
    bool uiFinalSelected_ {false};
    bool uiDetailsVisible_ {false};

    VSTGUI::VST3Editor* editor_ {nullptr};
    VSTGUI::CControl* mixControl_ {nullptr};
    VSTGUI::CControl* masterControl_ {nullptr};
    VSTGUI::CControl* liveControl_ {nullptr};
    VSTGUI::CControl* finalControl_ {nullptr};
    VSTGUI::COptionMenu* genreMenu_ {nullptr};
    VSTGUI::COptionMenu* eraMenu_ {nullptr};
    VSTGUI::CView* simplePage_ {nullptr};
    VSTGUI::CView* detailsPage_ {nullptr};

    VSTGUI::CTextLabel* technicalVerdict_ {nullptr};
    VSTGUI::CTextLabel* styleVerdict_ {nullptr};
    VSTGUI::CTextLabel* pcmVerdict_ {nullptr};
    VSTGUI::CTextLabel* streamingVerdict_ {nullptr};
    VSTGUI::CTextLabel* stateLabel_ {nullptr};
    VSTGUI::CTextLabel* overallVerdict_ {nullptr};
    VSTGUI::CTextLabel* overallLine1_ {nullptr};
    VSTGUI::CTextLabel* overallLine2_ {nullptr};
    VSTGUI::CTextLabel* integratedValue_ {nullptr};
    VSTGUI::CTextLabel* truePeakValue_ {nullptr};
    VSTGUI::CTextLabel* plrValue_ {nullptr};
    VSTGUI::CTextLabel* lraValue_ {nullptr};
    VSTGUI::CTextLabel* correlationValue_ {nullptr};
    VSTGUI::CTextLabel* monoValue_ {nullptr};
};
}
