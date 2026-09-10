#include "MixoratorController.h"
#include "../analysis/AssessmentInput.h"
#include "../dsp/AnalysisSnapshot.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/events.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace Mixorator
{
namespace
{
const VSTGUI::CPoint kCompactSize {650., 440.};
const VSTGUI::CPoint kDetailsSize {1000., 700.};
const VSTGUI::CColor kVerdictGood {120,210,170,255};
const VSTGUI::CColor kVerdictAttention {217,182,111,255};
const VSTGUI::CColor kVerdictCritical {224,103,103,255};
const VSTGUI::CColor kVerdictUnusual {105,215,192,255};
const VSTGUI::CColor kVerdictUnavailable {126,138,147,255};
const VSTGUI::CColor kStateLive {105,215,192,255};
const VSTGUI::CColor kStateFinal {120,210,170,255};
const VSTGUI::CColor kStatePending {217,182,111,255};
const VSTGUI::CColor kMetricAvailable {242,244,246,255};
const VSTGUI::CColor kMetricUnavailable {126,138,147,255};
const VSTGUI::CColor kAnalysisLight1 {255,193,73,255};
const VSTGUI::CColor kAnalysisLight2 {255,226,112,255};
const VSTGUI::CColor kAnalysisLight3 {231,239,105,255};
constexpr double kAnalysisLightStep = 0.035;
const VSTGUI::CRect kAnalysisLightRect {286., 51., 328., 75.};

const char* verdictText(Analysis::Verdict v) noexcept { switch(v){case Analysis::Verdict::Excellent:return "EXCELLENT";case Analysis::Verdict::Good:return "GOOD";case Analysis::Verdict::Attention:return "ATTENTION";case Analysis::Verdict::Critical:return "CRITICAL";case Analysis::Verdict::Unusual:return "UNUSUAL";case Analysis::Verdict::InsufficientData:return "N/A";} return "N/A"; }
const VSTGUI::CColor& verdictColor(Analysis::Verdict v) noexcept { switch(v){case Analysis::Verdict::Excellent:case Analysis::Verdict::Good:return kVerdictGood;case Analysis::Verdict::Attention:return kVerdictAttention;case Analysis::Verdict::Critical:return kVerdictCritical;case Analysis::Verdict::Unusual:return kVerdictUnusual;case Analysis::Verdict::InsufficientData:return kVerdictUnavailable;} return kVerdictUnavailable; }
void setLabel(VSTGUI::CTextLabel* l,const char* t) noexcept { if(!l)return;l->setText(t?t:"");l->invalid(); }
void setColoredLabel(VSTGUI::CTextLabel* l,const char* t,const VSTGUI::CColor& c) noexcept { if(!l)return;l->setText(t?t:"");l->setFontColor(c);l->invalid(); }
void setVerdictLabel(VSTGUI::CTextLabel* l,Analysis::Verdict v) noexcept { setColoredLabel(l,verdictText(v),verdictColor(v)); }
void formatValue(VSTGUI::CTextLabel* l,double value,const char* suffix,bool available,int precision=1) noexcept { if(!l)return;if(!available){l->setText("--");l->setFontColor(kMetricUnavailable);l->invalid();return;}char b[64]{};const bool s=suffix&&suffix[0]!='\0';if(precision==2)std::snprintf(b,sizeof(b),s?"%.2f %s":"%.2f",value,s?suffix:"");else std::snprintf(b,sizeof(b),s?"%.1f %s":"%.1f",value,s?suffix:"");l->setText(b);l->setFontColor(kMetricAvailable);l->invalid(); }
void expandSelectionHitArea(VSTGUI::CControl* c) noexcept { if(!c)return;auto r=c->getViewSize();r.left-=2.;r.right+=2.;r.top-=10.;r.bottom+=10.;c->setMouseableArea(r); }
void lockAnalysisLightGeometry(VSTGUI::CTextLabel* l) noexcept { if(!l)return;l->setViewSize(kAnalysisLightRect);l->setMouseableArea(kAnalysisLightRect); }
}

Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    hasPacket_=false;latestPacket_={};acceptFirstPacketAfterQueueOpen_=true;requestedFinalGeneration_=0;finalSnapshotGeneration_=0;uiMode_=Analysis::AnalysisMode::Mix;uiGenre_=Analysis::Genre::General;uiEra_=Analysis::Era::Modern;uiAnalysisActive_=false;uiFinalSelected_=false;uiDetailsVisible_=false;analysisLightPhase_=0.0;analysisLightTimer_=nullptr;clearUiPointers();return EditController::initialize(context);
}

Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name)
{
    if(name&&std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0){const char* viewName=uiDetailsVisible_?"detailsView":"compactView";auto* e=new VSTGUI::VST3Editor(this,viewName,"mixorator.uidesc");e->setDelegate(this);return e;}return nullptr;
}

VSTGUI::CView* Controller::verifyView(VSTGUI::CView* view,const VSTGUI::UIAttributes& attributes,const VSTGUI::IUIDescription*,VSTGUI::VST3Editor*)
{
    if(!view)return nullptr;bindNamedView(view,attributes);if(auto* c=dynamic_cast<VSTGUI::CControl*>(view)){switch(c->getTag()){case kUiMix:mixControl_=c;c->setListener(this);expandSelectionHitArea(c);c->registerViewEventListener(this);break;case kUiMaster:masterControl_=c;c->setListener(this);expandSelectionHitArea(c);c->registerViewEventListener(this);break;case kUiLive:liveControl_=c;c->setListener(this);expandSelectionHitArea(c);c->registerViewEventListener(this);break;case kUiFinal:finalControl_=c;c->setListener(this);expandSelectionHitArea(c);c->registerViewEventListener(this);break;case kUiGenre:if(auto* m=dynamic_cast<VSTGUI::COptionMenu*>(c)){genreMenu_=m;m->setListener(this);m->removeAllEntry();const char* n[]={"Rock","Metal","Pop","Techno","House / EDM","Drum & Bass","Hip-Hop / Trap","R&B / Soul","Electronic","Ambient","Acoustic / Folk","Jazz","Classical","Cinematic Music","General"};for(auto* x:n)m->addEntry(x);m->setCurrent(static_cast<std::int32_t>(uiGenre_));}break;case kUiEra:if(auto* m=dynamic_cast<VSTGUI::COptionMenu*>(c)){eraMenu_=m;m->setListener(this);m->removeAllEntry();m->addEntry("Modern");m->addEntry("Vintage");m->setCurrent(static_cast<std::int32_t>(uiEra_));}break;case kUiReset:case kUiAnalyze:case kUiDetails:case kUiBack:c->setListener(this);break;default:break;}}refreshUi();return view;
}

void Controller::didOpen(VSTGUI::VST3Editor* e){editor_=e;if(e)e->requestResize(uiDetailsVisible_?kDetailsSize:kCompactSize);if(hasPacket_&&latestPacket_.finalState!=0&&!hasDefinitiveFinalSnapshot()){requestedFinalGeneration_=0;requestFinalSnapshot(latestPacket_.finalizationGeneration);}refreshUi();}
void Controller::willClose(VSTGUI::VST3Editor*){analysisLightTimer_=nullptr;clearUiPointers();}
void Controller::viewOnEvent(VSTGUI::CView* view,VSTGUI::Event& event){if(event.type!=VSTGUI::EventType::MouseDown)return;auto* c=dynamic_cast<VSTGUI::CControl*>(view);if(!c)return;switch(c->getTag()){case kUiMix:case kUiMaster:case kUiLive:case kUiFinal:break;default:return;}auto& mouse=VSTGUI::castMouseDownEvent(event);if(!mouse.buttonState.has(VSTGUI::MouseButton::Left))return;valueChanged(c);mouse.consumed=true;mouse.ignoreFollowUpMoveAndUpEvents(true);}
void Controller::valueChanged(VSTGUI::CControl* c){if(!c)return;switch(c->getTag()){case kUiMix:uiMode_=Analysis::AnalysisMode::Mix;break;case kUiMaster:uiMode_=Analysis::AnalysisMode::Master;break;case kUiLive:case kUiAnalyze:if(requestLiveAnalysis()==Steinberg::kResultTrue){uiAnalysisActive_=true;uiFinalSelected_=false;hasPacket_=false;latestPacket_={};acceptFirstPacketAfterQueueOpen_=true;requestedFinalGeneration_=0;finalSnapshotGeneration_=0;analysisLightPhase_=0.0;}break;case kUiFinal:if(uiAnalysisActive_&&requestFinalAnalysis()==Steinberg::kResultTrue){uiAnalysisActive_=false;uiFinalSelected_=true;}break;case kUiGenre:if(genreMenu_){auto i=genreMenu_->getCurrentIndex();if(i>=0&&i<=static_cast<std::int32_t>(Analysis::Genre::General))uiGenre_=static_cast<Analysis::Genre>(i);}break;case kUiEra:if(eraMenu_){auto i=eraMenu_->getCurrentIndex();if(i>=0&&i<=static_cast<std::int32_t>(Analysis::Era::Vintage))uiEra_=static_cast<Analysis::Era>(i);}break;case kUiDetails:{uiDetailsVisible_=true;auto* e=editor_;clearUiPointers();editor_=e;if(e){e->exchangeView("detailsView");e->requestResize(kDetailsSize);}return;}case kUiBack:{uiDetailsVisible_=false;auto* e=editor_;clearUiPointers();editor_=e;if(e){e->exchangeView("compactView");e->requestResize(kCompactSize);}return;}case kUiReset:if(requestResetAnalysis()==Steinberg::kResultTrue){uiAnalysisActive_=false;uiFinalSelected_=false;hasPacket_=false;latestPacket_={};acceptFirstPacketAfterQueueOpen_=true;requestedFinalGeneration_=0;finalSnapshotGeneration_=0;analysisLightPhase_=0.0;}break;default:return;}refreshUi();}

void Controller::bindNamedView(VSTGUI::CView* view,const VSTGUI::UIAttributes& a) noexcept
{
    const auto* id=a.getAttributeValue("mixorator-id");if(!id)return;if(*id=="simplePage"){simplePage_=view;return;}if(*id=="detailsPage"){detailsPage_=view;return;}auto* l=dynamic_cast<VSTGUI::CTextLabel*>(view);if(!l)return;if(*id=="technicalVerdict")technicalVerdict_=l;else if(*id=="styleVerdict")styleVerdict_=l;else if(*id=="pcmVerdict")pcmVerdict_=l;else if(*id=="streamingVerdict")streamingVerdict_=l;else if(*id=="stateLabel")stateLabel_=l;else if(*id=="overallVerdict")overallVerdict_=l;else if(*id=="overallLine1")overallLine1_=l;else if(*id=="overallLine2")overallLine2_=l;else if(*id=="integratedValue")integratedValue_=l;else if(*id=="truePeakValue")truePeakValue_=l;else if(*id=="plrValue")plrValue_=l;else if(*id=="lraValue")lraValue_=l;else if(*id=="correlationValue")correlationValue_=l;else if(*id=="monoValue")monoValue_=l;else if(*id=="analysisLight1"){analysisLight1_=l;lockAnalysisLightGeometry(l);}else if(*id=="analysisLight2"){analysisLight2_=l;lockAnalysisLightGeometry(l);}else if(*id=="analysisLight3"){analysisLight3_=l;lockAnalysisLightGeometry(l);}
}

void Controller::updatePageVisibility() noexcept {if(simplePage_){simplePage_->setVisible(!uiDetailsVisible_);simplePage_->invalid();}if(detailsPage_){detailsPage_->setVisible(uiDetailsVisible_);detailsPage_->invalid();}}
void Controller::updateSelectionControls() noexcept {if(mixControl_){mixControl_->setValue(uiMode_==Analysis::AnalysisMode::Mix?1.f:0.f);mixControl_->invalid();}if(masterControl_){masterControl_->setValue(uiMode_==Analysis::AnalysisMode::Master?1.f:0.f);masterControl_->invalid();}if(liveControl_){liveControl_->setValue(uiAnalysisActive_&&!uiFinalSelected_?1.f:0.f);liveControl_->invalid();}if(finalControl_){finalControl_->setValue(uiFinalSelected_?1.f:0.f);finalControl_->invalid();}if(genreMenu_)genreMenu_->setCurrent(static_cast<std::int32_t>(uiGenre_));if(eraMenu_)eraMenu_->setCurrent(static_cast<std::int32_t>(uiEra_));updatePageVisibility();}

void Controller::positionAnalysisLights() noexcept
{
    VSTGUI::CTextLabel* lights[] = {analysisLight1_,analysisLight2_,analysisLight3_};
    const VSTGUI::CColor colors[] = {kAnalysisLight1,kAnalysisLight2,kAnalysisLight3};
    const double pulse = 0.16 + 0.82 * (0.5 + 0.5 * std::sin(analysisLightPhase_));
    for(int i=0;i<3;++i)
    {
        auto* light=lights[i];if(!light)continue;
        lockAnalysisLightGeometry(light);
        light->setFontColor(colors[i]);
        light->setAlphaValue(static_cast<float>(pulse));
        light->setVisible(true);
        light->invalid();
    }
}

void Controller::tickAnalysisLight() noexcept
{
    if(!uiAnalysisActive_||uiFinalSelected_)return;
    analysisLightPhase_+=kAnalysisLightStep;
    constexpr double twoPi=6.28318530717958647692;
    if(analysisLightPhase_>=twoPi)analysisLightPhase_-=twoPi;
    positionAnalysisLights();
}

void Controller::updateAnalysisLightState() noexcept
{
    const bool running=uiAnalysisActive_&&!uiFinalSelected_;
    if(running)
    {
        positionAnalysisLights();
        if(!analysisLightTimer_&&editor_)
            analysisLightTimer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){tickAnalysisLight();},33);
        return;
    }

    analysisLightTimer_=nullptr;
    if(uiFinalSelected_)
    {
        positionAnalysisLights();
        return;
    }

    if(analysisLight1_){analysisLight1_->setVisible(false);analysisLight1_->invalid();}
    if(analysisLight2_){analysisLight2_->setVisible(false);analysisLight2_->invalid();}
    if(analysisLight3_){analysisLight3_->setVisible(false);analysisLight3_->invalid();}
}

void Controller::refreshUi() noexcept {updateSelectionControls();updateAnalysisLightState();const auto a=evaluateLatest(uiMode_,uiGenre_,uiEra_);setVerdictLabel(technicalVerdict_,a.technicalVerdict);setVerdictLabel(styleVerdict_,a.styleVerdict);setVerdictLabel(pcmVerdict_,a.pcmDeliveryVerdict);setVerdictLabel(streamingVerdict_,a.streamingDeliveryVerdict);setVerdictLabel(overallVerdict_,a.overallVerdict);if(!hasPacket_){if(uiFinalSelected_){setColoredLabel(stateLabel_,"FINAL / PENDING",kStatePending);setLabel(overallLine1_,"Final result requested");setLabel(overallLine2_,"Start playback if processing is stopped");}else if(uiAnalysisActive_){setColoredLabel(stateLabel_,"LIVE / PROVISIONAL",kStateLive);setLabel(overallLine1_,"Play the complete song from the start");setLabel(overallLine2_,"When finished, press FINALIZE for result");}else{setColoredLabel(stateLabel_,"READY",kStateLive);setLabel(overallLine1_,"Choose MIX or MASTER, then ANALYZE");setLabel(overallLine2_,"Play the complete song from the start");}formatValue(integratedValue_,0,"LUFS",false);formatValue(truePeakValue_,0,"dBTP",false);formatValue(plrValue_,0,"dB",false);formatValue(lraValue_,0,"LU",false);formatValue(correlationValue_,0,"",false);formatValue(monoValue_,0,"dB",false);return;}const auto& m=latestPacket_.metrics;const bool fp=latestPacket_.finalState!=0;const bool definitive=fp&&hasDefinitiveFinalSnapshot();const bool pending=uiFinalSelected_&&!definitive;if(pending){setColoredLabel(stateLabel_,"FINAL / PENDING",kStatePending);setLabel(overallLine1_,"Final result requested");setLabel(overallLine2_,fp?"Preparing definitive result":"Waiting for processor finalize");}else if(definitive){setColoredLabel(stateLabel_,"FINAL / DEFINITIVE",kStateFinal);setLabel(overallLine1_,"Analysis complete - definitive result");setLabel(overallLine2_,"Press ANALYZE for a new measurement");}else{setColoredLabel(stateLabel_,"LIVE / PROVISIONAL",kStateLive);setLabel(overallLine1_,"Play the complete song from the start");setLabel(overallLine2_,"When finished, press FINALIZE for result");}const bool p=m.loudnessAvailable;formatValue(integratedValue_,m.integratedLufs,"LUFS",p&&m.integratedLufs>-999.0);formatValue(truePeakValue_,m.truePeakDbtp,"dBTP",p&&m.truePeakDbtp>-999.0);formatValue(plrValue_,m.plrDb,"dB",m.plrAvailable);formatValue(lraValue_,m.lraLu,"LU",m.lraAvailable);formatValue(correlationValue_,m.correlation,"",p,2);formatValue(monoValue_,m.monoCompatibilityDb,"dB",p);}
void Controller::clearUiPointers() noexcept {editor_=nullptr;mixControl_=nullptr;masterControl_=nullptr;liveControl_=nullptr;finalControl_=nullptr;genreMenu_=nullptr;eraMenu_=nullptr;simplePage_=nullptr;detailsPage_=nullptr;technicalVerdict_=nullptr;styleVerdict_=nullptr;pcmVerdict_=nullptr;streamingVerdict_=nullptr;stateLabel_=nullptr;overallVerdict_=nullptr;overallLine1_=nullptr;overallLine2_=nullptr;integratedValue_=nullptr;truePeakValue_=nullptr;plrValue_=nullptr;lraValue_=nullptr;correlationValue_=nullptr;monoValue_=nullptr;analysisLight1_=nullptr;analysisLight2_=nullptr;analysisLight3_=nullptr;}
Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage* m){if(consumeFinalSnapshotMessage(m))return Steinberg::kResultTrue;if(dataExchange_.onMessage(m))return Steinberg::kResultTrue;return EditController::notify(m);}
Steinberg::tresult Controller::requestAnalysisState(Steinberg::int64 state) noexcept {if(state!=kAnalysisStateIdle&&state!=kAnalysisStateLive&&state!=kAnalysisStateFinal)return Steinberg::kInvalidArgument;auto* m=allocateMessage();if(!m)return Steinberg::kOutOfMemory;m->setMessageID(kSetAnalysisStateMessage);Steinberg::tresult r=Steinberg::kResultFalse;if(auto* a=m->getAttributes())if(a->setInt(kAnalysisStateKey,state)==Steinberg::kResultTrue)r=sendMessage(m);m->release();return r;}
Steinberg::tresult Controller::requestResetAnalysis() noexcept{return requestAnalysisState(kAnalysisStateIdle);}
Steinberg::tresult Controller::requestLiveAnalysis() noexcept{return requestAnalysisState(kAnalysisStateLive);}
Steinberg::tresult Controller::requestFinalAnalysis() noexcept{return requestAnalysisState(kAnalysisStateFinal);}
bool Controller::consumeFinalSnapshotMessage(Steinberg::Vst::IMessage* m) noexcept {if(!m||!m->getMessageID()||std::strcmp(m->getMessageID(),kFinalSnapshotMessage)!=0)return false;auto* a=m->getAttributes();if(!a)return true;Steinberg::int64 gv=0;const void* data=nullptr;Steinberg::uint32 size=0;if(a->getInt(kFinalSnapshotGenerationKey,gv)!=Steinberg::kResultTrue||a->getBinary(kFinalSnapshotDataKey,data,size)!=Steinberg::kResultTrue||gv<0||!data||size!=sizeof(DSP::AnalysisSnapshot)){requestedFinalGeneration_=0;return true;}const auto g=static_cast<std::uint64_t>(gv);if(!hasPacket_||latestPacket_.finalState==0||latestPacket_.finalizationGeneration!=g){requestedFinalGeneration_=0;return true;}DSP::AnalysisSnapshot s;std::memcpy(&s,data,sizeof(s));if(!s.valid){requestedFinalGeneration_=0;return true;}latestPacket_.metrics=Analysis::AssessmentInput::fromFinal(s);finalSnapshotGeneration_=g;refreshUi();return true;}
void Controller::requestFinalSnapshot(std::uint64_t g) noexcept {if(g==0||g==requestedFinalGeneration_)return;auto* m=allocateMessage();if(!m)return;m->setMessageID(kRequestFinalSnapshotMessage);if(auto* a=m->getAttributes()){a->setInt(kFinalSnapshotGenerationKey,static_cast<Steinberg::int64>(g));if(sendMessage(m)==Steinberg::kResultTrue)requestedFinalGeneration_=g;}m->release();}
void PLUGIN_API Controller::queueOpened(Steinberg::Vst::DataExchangeUserContextID id,Steinberg::uint32 size,Steinberg::TBool& bg){if(id!=kAnalysisExchangeContext||size<sizeof(AnalysisExchangePacket))return;bg=false;acceptFirstPacketAfterQueueOpen_=true;if(hasPacket_&&latestPacket_.finalState!=0&&!hasDefinitiveFinalSnapshot()){requestedFinalGeneration_=0;requestFinalSnapshot(latestPacket_.finalizationGeneration);}}
void PLUGIN_API Controller::queueClosed(Steinberg::Vst::DataExchangeUserContextID id){if(id==kAnalysisExchangeContext)acceptFirstPacketAfterQueueOpen_=true;}
void PLUGIN_API Controller::onDataExchangeBlocksReceived(Steinberg::Vst::DataExchangeUserContextID id,Steinberg::uint32 n,Steinberg::Vst::DataExchangeBlock* blocks,Steinberg::TBool){if(id!=kAnalysisExchangeContext||!blocks)return;bool changed=false;for(Steinberg::uint32 i=0;i<n;++i){if(!blocks[i].data||blocks[i].size<sizeof(AnalysisExchangePacket))continue;AnalysisExchangePacket p;std::memcpy(&p,blocks[i].data,sizeof(p));const bool sequenceEpochStart=acceptFirstPacketAfterQueueOpen_;if(!sequenceEpochStart&&hasPacket_&&p.sequence<latestPacket_.sequence)continue;acceptFirstPacketAfterQueueOpen_=false;const bool preserveLastValidMetrics=hasPacket_&&latestPacket_.metrics.loudnessAvailable&&!p.metrics.loudnessAvailable;if(preserveLastValidMetrics){const auto previousMetrics=latestPacket_.metrics;latestPacket_=p;latestPacket_.metrics=previousMetrics;}else latestPacket_=p;hasPacket_=true;changed=true;if(p.finalState!=0){uiAnalysisActive_=false;uiFinalSelected_=true;requestFinalSnapshot(p.finalizationGeneration);}else{uiAnalysisActive_=true;uiFinalSelected_=false;requestedFinalGeneration_=0;finalSnapshotGeneration_=0;}}if(changed)refreshUi();}
Analysis::Assessment Controller::evaluateLatest(Analysis::AnalysisMode mode,Analysis::Genre genre,Analysis::Era era) const noexcept {if(!hasPacket_){Analysis::Metrics u;u.loudnessAvailable=false;u.plrAvailable=false;u.lraAvailable=false;u.provisional=true;return Analysis::AssessmentModel::evaluate(u,mode,genre,era);}if(latestPacket_.finalState!=0&&!hasDefinitiveFinalSnapshot()){auto w=latestPacket_.metrics;w.provisional=true;return Analysis::AssessmentModel::evaluate(w,mode,genre,era);}return Analysis::AssessmentModel::evaluate(latestPacket_.metrics,mode,genre,era);}
}
