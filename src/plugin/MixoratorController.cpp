#include "MixoratorController.h"
#include "../analysis/AssessmentInput.h"
#include "../dsp/AnalysisSnapshot.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/controls/ctextlabel.h"

#include <cstdio>
#include <cstring>

namespace Mixorator
{
namespace
{
const VSTGUI::CPoint kCompactSize {650., 440.};
const VSTGUI::CPoint kDetailsSize {1000., 700.};

const char* verdictText(Analysis::Verdict verdict) noexcept
{
    switch (verdict)
    {
        case Analysis::Verdict::Excellent: return "EXCELLENT";
        case Analysis::Verdict::Good: return "GOOD";
        case Analysis::Verdict::Attention: return "ATTENTION";
        case Analysis::Verdict::Critical: return "CRITICAL";
        case Analysis::Verdict::Unusual: return "UNUSUAL";
        case Analysis::Verdict::InsufficientData: return "N/A";
    }
    return "N/A";
}
void setLabel(VSTGUI::CTextLabel* label, const char* text) noexcept { if (label) label->setText(text ? text : ""); }
void formatValue(VSTGUI::CTextLabel* label, double value, const char* suffix, bool available, int precision = 1) noexcept
{
    if (!label) return;
    if (!available) { label->setText("--"); return; }
    char buffer[64] {};
    std::snprintf(buffer, sizeof(buffer), precision == 2 ? "%.2f %s" : "%.1f %s", value, suffix ? suffix : "");
    label->setText(buffer);
}
}

Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    hasPacket_ = false; latestPacket_ = {}; requestedFinalGeneration_ = 0; finalSnapshotGeneration_ = 0;
    uiMode_ = Analysis::AnalysisMode::Mix; uiGenre_ = Analysis::Genre::General; uiEra_ = Analysis::Era::Modern;
    uiFinalSelected_ = false; uiDetailsVisible_ = false; clearUiPointers();
    return EditController::initialize(context);
}
Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name)
{
    if (name && std::strcmp(name, Steinberg::Vst::ViewType::kEditor) == 0) { auto* editor = new VSTGUI::VST3Editor(this, "compactView", "mixorator.uidesc"); editor->setDelegate(this); return editor; }
    return nullptr;
}
VSTGUI::CView* Controller::verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription*, VSTGUI::VST3Editor*)
{
    if (!view) return nullptr; bindNamedView(view, attributes);
    if (auto* control = dynamic_cast<VSTGUI::CControl*>(view))
    {
        switch (control->getTag())
        {
            case kUiMix: mixControl_ = control; control->setListener(this); break;
            case kUiMaster: masterControl_ = control; control->setListener(this); break;
            case kUiLive: liveControl_ = control; control->setListener(this); break;
            case kUiFinal: finalControl_ = control; control->setListener(this); break;
            case kUiGenre:
                if (auto* menu = dynamic_cast<VSTGUI::COptionMenu*>(control)) { genreMenu_ = menu; menu->setListener(this); menu->removeAllEntry(); const char* names[] = {"Rock","Metal","Pop","Techno","House / EDM","Hip-Hop / Trap","Electronic / Ambient","Acoustic / Folk","Jazz","Classical","Cinematic Music","General"}; for (auto* n : names) menu->addEntry(n); menu->setCurrent(static_cast<std::int32_t>(uiGenre_)); } break;
            case kUiEra:
                if (auto* menu = dynamic_cast<VSTGUI::COptionMenu*>(control)) { eraMenu_ = menu; menu->setListener(this); menu->removeAllEntry(); menu->addEntry("Modern"); menu->addEntry("Vintage"); menu->setCurrent(static_cast<std::int32_t>(uiEra_)); } break;
            case kUiReset: case kUiAnalyze: case kUiDetails: case kUiBack: control->setListener(this); break;
            default: break;
        }
    }
    return view;
}
void Controller::didOpen(VSTGUI::VST3Editor* editor)
{
    editor_ = editor;
    refreshUi();
}
void Controller::willClose(VSTGUI::VST3Editor*) { clearUiPointers(); }
void Controller::valueChanged(VSTGUI::CControl* control)
{
    if (!control) return;
    switch (control->getTag())
    {
        case kUiMix: uiMode_ = Analysis::AnalysisMode::Mix; break;
        case kUiMaster: uiMode_ = Analysis::AnalysisMode::Master; break;
        case kUiLive: if (requestLiveAnalysis() == Steinberg::kResultTrue) { uiFinalSelected_ = false; hasPacket_ = false; latestPacket_ = {}; requestedFinalGeneration_ = 0; finalSnapshotGeneration_ = 0; } break;
        case kUiFinal: if (requestFinalAnalysis() == Steinberg::kResultTrue) uiFinalSelected_ = true; break;
        case kUiGenre: if (genreMenu_) { const auto i = genreMenu_->getCurrentIndex(); if (i >= 0 && i <= static_cast<std::int32_t>(Analysis::Genre::General)) uiGenre_ = static_cast<Analysis::Genre>(i); } break;
        case kUiEra: if (eraMenu_) { const auto i = eraMenu_->getCurrentIndex(); if (i >= 0 && i <= static_cast<std::int32_t>(Analysis::Era::Vintage)) uiEra_ = static_cast<Analysis::Era>(i); } break;
        case kUiDetails:
        {
            uiDetailsVisible_ = true;
            auto* currentEditor = editor_;
            clearUiPointers();
            editor_ = currentEditor;
            if (currentEditor)
            {
                currentEditor->exchangeView("detailsView");
                currentEditor->requestResize(kDetailsSize);
            }
            return;
        }
        case kUiBack:
        {
            uiDetailsVisible_ = false;
            auto* currentEditor = editor_;
            clearUiPointers();
            editor_ = currentEditor;
            if (currentEditor)
            {
                currentEditor->exchangeView("compactView");
                currentEditor->requestResize(kCompactSize);
            }
            return;
        }
        case kUiReset: case kUiAnalyze: if (requestLiveAnalysis() == Steinberg::kResultTrue) { uiFinalSelected_ = false; hasPacket_ = false; latestPacket_ = {}; requestedFinalGeneration_ = 0; finalSnapshotGeneration_ = 0; } break;
        default: return;
    }
    refreshUi();
}
void Controller::bindNamedView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes) noexcept
{
    const auto* id = attributes.getAttributeValue("mixorator-id"); if (!id) return;
    if (*id == "simplePage") { simplePage_ = view; return; } if (*id == "detailsPage") { detailsPage_ = view; return; }
    auto* label = dynamic_cast<VSTGUI::CTextLabel*>(view); if (!label) return;
    if (*id == "technicalVerdict") technicalVerdict_ = label; else if (*id == "styleVerdict") styleVerdict_ = label;
    else if (*id == "pcmVerdict") pcmVerdict_ = label; else if (*id == "streamingVerdict") streamingVerdict_ = label;
    else if (*id == "stateLabel") stateLabel_ = label; else if (*id == "overallVerdict") overallVerdict_ = label;
    else if (*id == "overallLine1") overallLine1_ = label; else if (*id == "overallLine2") overallLine2_ = label;
    else if (*id == "integratedValue") integratedValue_ = label; else if (*id == "truePeakValue") truePeakValue_ = label;
    else if (*id == "plrValue") plrValue_ = label; else if (*id == "lraValue") lraValue_ = label;
    else if (*id == "correlationValue") correlationValue_ = label; else if (*id == "monoValue") monoValue_ = label;
}
void Controller::updatePageVisibility() noexcept
{
    if (simplePage_) { simplePage_->setVisible(!uiDetailsVisible_); simplePage_->invalid(); }
    if (detailsPage_) { detailsPage_->setVisible(uiDetailsVisible_); detailsPage_->invalid(); }
}
void Controller::updateSelectionControls() noexcept
{
    if (mixControl_) { mixControl_->setValue(uiMode_ == Analysis::AnalysisMode::Mix ? 1.f : 0.f); mixControl_->invalid(); }
    if (masterControl_) { masterControl_->setValue(uiMode_ == Analysis::AnalysisMode::Master ? 1.f : 0.f); masterControl_->invalid(); }
    if (liveControl_) { liveControl_->setValue(uiFinalSelected_ ? 0.f : 1.f); liveControl_->invalid(); }
    if (finalControl_) { finalControl_->setValue(uiFinalSelected_ ? 1.f : 0.f); finalControl_->invalid(); }
    if (genreMenu_) genreMenu_->setCurrent(static_cast<std::int32_t>(uiGenre_)); if (eraMenu_) eraMenu_->setCurrent(static_cast<std::int32_t>(uiEra_)); updatePageVisibility();
}
void Controller::refreshUi() noexcept
{
    updateSelectionControls(); const auto assessment = evaluateLatest(uiMode_, uiGenre_, uiEra_);
    setLabel(technicalVerdict_, verdictText(assessment.technicalVerdict)); setLabel(styleVerdict_, verdictText(assessment.styleVerdict));
    setLabel(pcmVerdict_, verdictText(assessment.pcmDeliveryVerdict)); setLabel(streamingVerdict_, verdictText(assessment.streamingDeliveryVerdict)); setLabel(overallVerdict_, verdictText(assessment.overallVerdict));
    if (!hasPacket_)
    {
        setLabel(stateLabel_, "WAITING FOR AUDIO"); setLabel(overallLine1_, "Start playback to analyze"); setLabel(overallLine2_, "Waiting for programme data");
        formatValue(integratedValue_,0,"LUFS",false); formatValue(truePeakValue_,0,"dBTP",false); formatValue(plrValue_,0,"dB",false); formatValue(lraValue_,0,"LU",false); formatValue(correlationValue_,0,"",false); formatValue(monoValue_,0,"dB",false); return;
    }
    const auto& metrics = latestPacket_.metrics; const bool finalPacket = latestPacket_.finalState != 0; const bool definitive = finalPacket && hasDefinitiveFinalSnapshot();
    if (finalPacket && !definitive) { setLabel(stateLabel_,"FINAL / PENDING"); setLabel(overallLine1_,"Final snapshot requested"); setLabel(overallLine2_,"Waiting for programme metrics"); }
    else if (definitive) { setLabel(stateLabel_,"FINAL / DEFINITIVE"); setLabel(overallLine1_,"Definitive programme assessment"); setLabel(overallLine2_,"Snapshot frozen until restart"); }
    else { setLabel(stateLabel_,"LIVE / PROVISIONAL"); setLabel(overallLine1_,"Live analysis in progress"); setLabel(overallLine2_,"Provisional until FINAL"); }
    const bool programmeAvailable = metrics.loudnessAvailable;
    formatValue(integratedValue_,metrics.integratedLufs,"LUFS",programmeAvailable && metrics.integratedLufs > -999.0); formatValue(truePeakValue_,metrics.truePeakDbtp,"dBTP",programmeAvailable && metrics.truePeakDbtp > -999.0);
    formatValue(plrValue_,metrics.plrDb,"dB",metrics.plrAvailable); formatValue(lraValue_,metrics.lraLu,"LU",metrics.lraAvailable); formatValue(correlationValue_,metrics.correlation,"",programmeAvailable,2); formatValue(monoValue_,metrics.monoCompatibilityDb,"dB",programmeAvailable);
}
void Controller::clearUiPointers() noexcept
{
    editor_=nullptr; mixControl_=nullptr; masterControl_=nullptr; liveControl_=nullptr; finalControl_=nullptr; genreMenu_=nullptr; eraMenu_=nullptr; simplePage_=nullptr; detailsPage_=nullptr;
    technicalVerdict_=nullptr; styleVerdict_=nullptr; pcmVerdict_=nullptr; streamingVerdict_=nullptr; stateLabel_=nullptr; overallVerdict_=nullptr; overallLine1_=nullptr; overallLine2_=nullptr; integratedValue_=nullptr; truePeakValue_=nullptr; plrValue_=nullptr; lraValue_=nullptr; correlationValue_=nullptr; monoValue_=nullptr;
}
Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage* message) { if (consumeFinalSnapshotMessage(message)) return Steinberg::kResultTrue; if (dataExchange_.onMessage(message)) return Steinberg::kResultTrue; return EditController::notify(message); }
Steinberg::tresult Controller::requestAnalysisState(Steinberg::int64 state) noexcept
{
    if (state != kAnalysisStateLive && state != kAnalysisStateFinal) return Steinberg::kInvalidArgument; auto* message=allocateMessage(); if(!message) return Steinberg::kOutOfMemory; message->setMessageID(kSetAnalysisStateMessage); Steinberg::tresult result=Steinberg::kResultFalse;
    if(auto* attributes=message->getAttributes()) if(attributes->setInt(kAnalysisStateKey,state)==Steinberg::kResultTrue) result=sendMessage(message); message->release(); return result;
}
Steinberg::tresult Controller::requestLiveAnalysis() noexcept { return requestAnalysisState(kAnalysisStateLive); }
Steinberg::tresult Controller::requestFinalAnalysis() noexcept { return requestAnalysisState(kAnalysisStateFinal); }
bool Controller::consumeFinalSnapshotMessage(Steinberg::Vst::IMessage* message) noexcept
{
    if(!message || !message->getMessageID() || std::strcmp(message->getMessageID(),kFinalSnapshotMessage)!=0) return false; auto* attributes=message->getAttributes(); if(!attributes) return true;
    Steinberg::int64 generationValue=0; const void* data=nullptr; Steinberg::uint32 size=0; if(attributes->getInt(kFinalSnapshotGenerationKey,generationValue)!=Steinberg::kResultTrue || attributes->getBinary(kFinalSnapshotDataKey,data,size)!=Steinberg::kResultTrue || generationValue<0 || !data || size!=sizeof(DSP::AnalysisSnapshot)) return true;
    const auto generation=static_cast<std::uint64_t>(generationValue); if(!hasPacket_ || latestPacket_.finalState==0 || latestPacket_.finalizationGeneration!=generation) return true; DSP::AnalysisSnapshot snapshot; std::memcpy(&snapshot,data,sizeof(snapshot)); if(!snapshot.valid) return true;
    latestPacket_.metrics=Analysis::AssessmentInput::fromFinal(snapshot); finalSnapshotGeneration_=generation; refreshUi(); return true;
}
void Controller::requestFinalSnapshot(std::uint64_t generation) noexcept
{
    if(generation==0 || generation==requestedFinalGeneration_) return; auto* message=allocateMessage(); if(!message) return; message->setMessageID(kRequestFinalSnapshotMessage);
    if(auto* attributes=message->getAttributes()) { attributes->setInt(kFinalSnapshotGenerationKey,static_cast<Steinberg::int64>(generation)); if(sendMessage(message)==Steinberg::kResultTrue) requestedFinalGeneration_=generation; } message->release();
}
void PLUGIN_API Controller::queueOpened(Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 blockSize, Steinberg::TBool& dispatchOnBackgroundThread) { if(userContextID==kAnalysisExchangeContext && blockSize>=sizeof(AnalysisExchangePacket)) dispatchOnBackgroundThread=false; }
void PLUGIN_API Controller::queueClosed(Steinberg::Vst::DataExchangeUserContextID userContextID) { if(userContextID==kAnalysisExchangeContext) { hasPacket_=false; requestedFinalGeneration_=0; finalSnapshotGeneration_=0; refreshUi(); } }
void PLUGIN_API Controller::onDataExchangeBlocksReceived(Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 numBlocks, Steinberg::Vst::DataExchangeBlock* blocks, Steinberg::TBool)
{
    if(userContextID!=kAnalysisExchangeContext || !blocks) return; bool changed=false; for(Steinberg::uint32 i=0;i<numBlocks;++i) { if(!blocks[i].data || blocks[i].size<sizeof(AnalysisExchangePacket)) continue; AnalysisExchangePacket packet; std::memcpy(&packet,blocks[i].data,sizeof(packet)); if(hasPacket_ && packet.sequence<latestPacket_.sequence) continue; latestPacket_=packet; hasPacket_=true; uiFinalSelected_=packet.finalState!=0; changed=true; if(packet.finalState!=0) requestFinalSnapshot(packet.finalizationGeneration); else { requestedFinalGeneration_=0; finalSnapshotGeneration_=0; } } if(changed) refreshUi();
}
Analysis::Assessment Controller::evaluateLatest(Analysis::AnalysisMode mode, Analysis::Genre genre, Analysis::Era era) const noexcept
{
    if(!hasPacket_) { Analysis::Metrics unavailable; unavailable.loudnessAvailable=false; unavailable.plrAvailable=false; unavailable.lraAvailable=false; unavailable.provisional=true; return Analysis::AssessmentModel::evaluate(unavailable,mode,genre,era); }
    if(latestPacket_.finalState!=0 && !hasDefinitiveFinalSnapshot()) { auto waiting=latestPacket_.metrics; waiting.loudnessAvailable=false; waiting.plrAvailable=false; waiting.lraAvailable=false; waiting.provisional=true; return Analysis::AssessmentModel::evaluate(waiting,mode,genre,era); }
    return Analysis::AssessmentModel::evaluate(latestPacket_.metrics,mode,genre,era);
}
}
