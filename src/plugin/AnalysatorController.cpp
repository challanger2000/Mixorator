#include "AnalysatorController.h"
#include "../analysis/AssessmentInput.h"
#include "../analysis/AssessmentDiagnosisText.h"
#include "../dsp/AnalysisSnapshot.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/events.h"

#include <cstdio>
#include <cstring>

namespace Analysator
{
namespace
{
constexpr double kZoom68 = 1.0;   // native GUI = 100%
constexpr double kZoom100 = 1.5;  // enlarged GUI = 150%
constexpr std::int32_t kUiZoomTag = 10013;

class AnalysatorEditor final : public VSTGUI::VST3Editor
{
public:
    AnalysatorEditor(Steinberg::Vst::EditController* controller,
                     const char* viewName,
                     const char* xmlFile,
                     const VSTGUI::CPoint& nativeSize)
    : VSTGUI::VST3Editor(controller, viewName, xmlFile), nativeSize_(nativeSize) {}

    bool setUserZoom(double factor)
    {
        if (factor != kZoom68 && factor != kZoom100)
            return false;
        setZoomFactor(factor);
        setEditorSizeConstrains(nativeSize_, nativeSize_);
        return true;
    }

    void setNativeSize(const VSTGUI::CPoint& nativeSize)
    {
        nativeSize_ = nativeSize;
        setEditorSizeConstrains(nativeSize_, nativeSize_);
    }

    bool isZoom100() const noexcept { return getZoomFactor() > 1.25; }

    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description) override;

private:
    VSTGUI::CPoint nativeSize_;
};

class ZoomView final : public VSTGUI::CView
{
public:
    ZoomView(const VSTGUI::CRect& r, AnalysatorEditor* editor)
    : VSTGUI::CView(r), editor_(editor) { setMouseEnabled(true); }

    void draw(VSTGUI::CDrawContext* ctx) override
    {
        if (!ctx || !editor_) { setDirty(false); return; }
        auto r = getViewSize();
        ctx->setFont(VSTGUI::kNormalFontSmall);
        ctx->setFontColor(VSTGUI::CColor(169, 177, 183, 255));
        ctx->drawString(editor_->getZoomFactor() > 1.25 ? "150%" : "100%", r, VSTGUI::kCenterText);
        setDirty(false);
    }

    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&, const VSTGUI::CButtonState&) override
    {
        if (!editor_) return VSTGUI::kMouseEventNotHandled;
        editor_->setUserZoom(editor_->getZoomFactor() > 1.25 ? kZoom68 : kZoom100);
        invalid();
        return VSTGUI::kMouseEventHandled;
    }
private:
    AnalysatorEditor* editor_ {};
};

VSTGUI::CView* AnalysatorEditor::createView(const VSTGUI::UIAttributes& attributes,
                                             const VSTGUI::IUIDescription* description)
{
    if (const auto name = attributes.getAttributeValue(VSTGUI::IUIDescription::kCustomViewName))
    {
        if (*name == "UiZoomCompact") return new ZoomView({478., 12., 540., 32.}, this);
        if (*name == "UiZoomDetails") return new ZoomView({914., 6., 980., 24.}, this);
    }
    return VSTGUI::VST3Editor::createView(attributes, description);
}
// Verdict palette follows the assessment ring: green -> yellow-green -> amber -> red.
const VSTGUI::CColor kVerdictExcellent {73,196,112,255};
const VSTGUI::CColor kVerdictGood {174,205,89,255};
const VSTGUI::CColor kVerdictAttention {225,158,73,255};
const VSTGUI::CColor kVerdictCritical {224,91,72,255};
const VSTGUI::CColor kVerdictUnusual {105,215,192,255};
const VSTGUI::CColor kVerdictUnavailable {126,138,147,255};
const VSTGUI::CColor kStateLive {105,215,192,255};
const VSTGUI::CColor kStateFinal {120,210,170,255};
const VSTGUI::CColor kStatePending {217,182,111,255};
const VSTGUI::CColor kMetricAvailable {242,244,246,255};
const VSTGUI::CColor kMetricUnavailable {126,138,147,255};
const VSTGUI::CColor kLedOff {58,20,18,255};
const VSTGUI::CColor kLedOn {255,92,76,255};

VSTGUI::CColor blendLedColor(const VSTGUI::CColor& a, const VSTGUI::CColor& b, double amount) noexcept
{
    if (amount < 0.0) amount = 0.0;
    if (amount > 1.0) amount = 1.0;
    const auto mix = [amount](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(static_cast<double>(x) +
                                         (static_cast<double>(y) - static_cast<double>(x)) * amount + 0.5);
    };
    return {mix(a.red, b.red), mix(a.green, b.green), mix(a.blue, b.blue), mix(a.alpha, b.alpha)};
}

const char* verdictText(Analysis::Verdict v, Localization::Language language) noexcept
{
    if (language == Localization::Language::German)
    {
        switch (v)
        {
            case Analysis::Verdict::Excellent: return "EXZELLENT";
            case Analysis::Verdict::Good: return "GUT";
            case Analysis::Verdict::Attention: return "ACHTUNG";
            case Analysis::Verdict::Critical: return "KRITISCH";
            case Analysis::Verdict::Unusual: return "UNGEWÖHNLICH";
            case Analysis::Verdict::InsufficientData: return "N/A";
        }
    }
    switch (v)
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

const VSTGUI::CColor& verdictColor(Analysis::Verdict v) noexcept
{
    switch (v)
    {
        case Analysis::Verdict::Excellent: return kVerdictExcellent;
        case Analysis::Verdict::Good: return kVerdictGood;
        case Analysis::Verdict::Attention: return kVerdictAttention;
        case Analysis::Verdict::Critical: return kVerdictCritical;
        case Analysis::Verdict::Unusual: return kVerdictUnusual;
        case Analysis::Verdict::InsufficientData: return kVerdictUnavailable;
    }
    return kVerdictUnavailable;
}

void setLabel(VSTGUI::CTextLabel* l, const char* t) noexcept
{
    if (!l) return;
    l->setText(t ? t : "");
    l->invalid();
}

void setButtonTitle(VSTGUI::CTextButton* b, const char* t) noexcept
{
    if (!b) return;
    b->setTitle(t ? t : "");
    b->invalid();
}

void setColoredLabel(VSTGUI::CTextLabel* l, const char* t, const VSTGUI::CColor& c) noexcept
{
    if (!l) return;
    l->setText(t ? t : "");
    l->setFontColor(c);
    l->invalid();
}

void setVerdictLabel(VSTGUI::CTextLabel* l, Analysis::Verdict v, Localization::Language language) noexcept
{
    setColoredLabel(l, verdictText(v, language), verdictColor(v));
}

void formatValue(VSTGUI::CTextLabel* l, double value, const char* suffix, bool available, int precision = 1) noexcept
{
    if (!l) return;
    if (!available)
    {
        l->setText("--");
        l->setFontColor(kMetricUnavailable);
        l->invalid();
        return;
    }
    char b[64] {};
    const bool s = suffix && suffix[0] != '\0';
    if (precision == 2)
        std::snprintf(b, sizeof(b), s ? "%.2f %s" : "%.2f", value, s ? suffix : "");
    else
        std::snprintf(b, sizeof(b), s ? "%.1f %s" : "%.1f", value, s ? suffix : "");
    l->setText(b);
    l->setFontColor(kMetricAvailable);
    l->invalid();
}

void setFinalDiagnosis(VSTGUI::CTextLabel* line1,
                       VSTGUI::CTextLabel* line2,
                       const Analysis::Metrics& m,
                       const Analysis::Assessment& a,
                       Analysis::AnalysisMode mode,
                       Localization::Language language) noexcept
{
    const bool de = language == Localization::Language::German;

    if (m.nonFiniteSamples > 0)
    {
        setLabel(line1, de ? "Ungültige Audio-Samples erkannt." : "Invalid audio samples detected.");
        setLabel(line2, de ? "Signalweg/Export prüfen und erneut analysieren." : "Check the signal path/export and analyze again.");
        return;
    }
    if (m.clippedSamples > 0 && m.truePeakDbtp > 0.0)
    {
        setLabel(line1, de ? "Full-Scale-/True-Peak-Problem erkannt." : "Full-scale/true-peak ceiling issue detected.");
        setLabel(line2, de ? "Limiter/Output-Ceiling und Signalweg prüfen; Ursache nicht eindeutig." : "Check limiter/output ceiling and signal path; origin is not unambiguous.");
        return;
    }
    if (m.clippedSamples > 0)
    {
        setLabel(line1, de ? "Full-Scale-Plateaus erkannt: mögliches Clipping." : "Full-scale plateaus detected: possible clipping.");
        setLabel(line2, de ? "Limiter, Pegel und Signalweg gezielt prüfen." : "Check limiter, level and signal path.");
        return;
    }
    if (m.truePeakDbtp > 0.0)
    {
        setLabel(line1, de ? "True-Peak-Overs erkannt: Inter-Sample-Spitzen über 0 dBTP." : "True-peak overs detected: inter-sample peaks above 0 dBTP.");
        setLabel(line2, de ? "True-Peak-Limiter/Output-Ceiling absenken und erneut prüfen." : "Lower the true-peak limiter/output ceiling and check again.");
        return;
    }

    if (a.technicalVerdict != Analysis::Verdict::Excellent &&
        (m.correlation < 0.0 || m.monoCompatibilityDb < -3.0 ||
         m.worstLocalCorrelation < -0.2 || m.worstLocalMonoCompatibilityDb < -6.0))
    {
        setLabel(line1, de ? "Stereo-/Monokompatibilität ist auffällig." : "Stereo/mono compatibility is problematic.");
        setLabel(line2, de ? "Phase, Breite und Seitensignal gezielt kontrollieren." : "Check phase, width and side content.");
        return;
    }
    if (mode == Analysis::AnalysisMode::Master && m.plrAvailable &&
        m.integratedLufs > -8.0 && m.plrDb < 8.0)
    {
        setLabel(line1, de ? "Hohe Lautheit + niedriger PLR: stark verdichtetes Master." : "High loudness + low PLR: strongly dense master.");
        setLabel(line2, de ? "Stilistisch möglich; falls unbeabsichtigt Limiting/Kompression reduzieren." : "May be intentional; otherwise ease limiting/compression.");
        return;
    }
    if (mode == Analysis::AnalysisMode::Master &&
        (a.streamingDeliveryVerdict == Analysis::Verdict::Attention ||
         a.streamingDeliveryVerdict == Analysis::Verdict::Critical))
    {
        setLabel(line1, de ? "Streaming-Reserve ist zu knapp." : "Streaming headroom is too small.");
        setLabel(line2, de ? "Mehr True-Peak-Reserve für Codec/Transcoding einplanen." : "Leave more true-peak headroom for codec/transcoding.");
        return;
    }

    const auto evidence = Analysis::primaryScoreNeutralDiagnosticEvidence(m, a);
    if (evidence != Analysis::DiagnosticEvidence::None)
    {
        const auto diagnosis = Analysis::diagnosticText(
            evidence,
            de ? Analysis::DiagnosticLanguage::German : Analysis::DiagnosticLanguage::English);
        setLabel(line1, diagnosis.line1);
        setLabel(line2, diagnosis.line2);
        return;
    }

    if (a.styleVerdict == Analysis::Verdict::Attention ||
        a.styleVerdict == Analysis::Verdict::Critical ||
        a.styleVerdict == Analysis::Verdict::Unusual)
    {
        setLabel(line1, de ? "Der gewählte Stil-/Era-Kontext wird nur schwach getroffen." : "The selected style/era profile is only weakly matched.");
        setLabel(line2, de ? "Genre/Era prüfen oder Dynamik/Tonalität gezielt vergleichen." : "Check genre/era or compare dynamics/tonality.");
        return;
    }
    if (a.technicalVerdict == Analysis::Verdict::Good)
    {
        setLabel(line1, de ? "Kleine technische Kompromisse erkannt, kein kritischer Fehler." : "Minor technical trade-offs detected; no critical fault.");
        setLabel(line2, de ? "Nur bei Bedarf optimieren - das Signal ist grundsätzlich brauchbar." : "Optimize only if needed; the signal is fundamentally usable.");
        return;
    }
    if (a.styleVerdict == Analysis::Verdict::Good)
    {
        setLabel(line1, de ? "Stil-Treffer ist gut, aber nicht exakt im gewählten Profil." : "Style match is good, but not exact for the selected profile.");
        setLabel(line2, de ? "Keine technische Korrektur allein deshalb erforderlich." : "No technical correction is required for that alone.");
        return;
    }

    setLabel(line1, de ? "Keine kritischen technischen Probleme erkannt." : "No critical technical issues detected.");
    setLabel(line2, mode == Analysis::AnalysisMode::Master
        ? (de ? "Das Master ist in diesem Zustand technisch verwendbar." : "The master is technically usable as delivered.")
        : (de ? "Der Mix ist in diesem Zustand technisch unauffällig." : "The mix is technically clean in its current state."));
}

void expandSelectionHitArea(VSTGUI::CControl* c) noexcept
{
    if (!c) return;
    auto r = c->getViewSize();
    r.left -= 2.; r.right += 2.; r.top -= 10.; r.bottom += 10.;
    c->setMouseableArea(r);
}
}

Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    hasPacket_ = false;
    latestPacket_ = {};
    acceptFirstPacketAfterQueueOpen_ = true;
    requestedFinalGeneration_ = 0;
    finalSnapshotGeneration_ = 0;
    uiMode_ = Analysis::AnalysisMode::Mix;
    uiGenre_ = Analysis::Genre::General;
    uiEra_ = Analysis::Era::Modern;
    uiLanguage_ = Localization::Language::German;
    uiAnalysisActive_ = false;
    uiFinalSelected_ = false;
    uiDetailsVisible_ = false;
    uiHelpVisible_ = false;
    clearUiPointers();
    return EditController::initialize(context);
}

Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name)
{
    if (name && std::strcmp(name, Steinberg::Vst::ViewType::kEditor) == 0)
    {
        const char* viewName = uiDetailsVisible_ ? "detailsView" : "compactView";
        const VSTGUI::CPoint nativeSize = uiDetailsVisible_ ? VSTGUI::CPoint{1000., 700.}
                                                            : VSTGUI::CPoint{650., 440.};
        auto* e = new AnalysatorEditor(this, viewName, "analysator.uidesc", nativeSize);
        e->setDelegate(this);
        e->setZoomFactor(kZoom68);
        e->setAllowedZoomFactors({kZoom68, kZoom100});
        e->setEditorSizeConstrains(nativeSize, nativeSize);
        return e;
    }
    return nullptr;
}

VSTGUI::CView* Controller::verifyView(VSTGUI::CView* view,
                                      const VSTGUI::UIAttributes& attributes,
                                      const VSTGUI::IUIDescription*,
                                      VSTGUI::VST3Editor* editor)
{
    if (!view) return nullptr;
    bindNamedView(view, attributes);

    if (auto* c = dynamic_cast<VSTGUI::CControl*>(view))
    {
        switch (c->getTag())
        {
            case kUiMix:
                mixControl_ = c; c->setListener(this); expandSelectionHitArea(c); c->registerViewEventListener(this); break;
            case kUiMaster:
                masterControl_ = c; c->setListener(this); expandSelectionHitArea(c); c->registerViewEventListener(this); break;
            case kUiLive:
                liveControl_ = c; c->setListener(this); expandSelectionHitArea(c); c->registerViewEventListener(this); break;
            case kUiFinal:
                finalControl_ = c; c->setListener(this); expandSelectionHitArea(c); c->registerViewEventListener(this); break;
            case kUiGenre:
                if (auto* m = dynamic_cast<VSTGUI::COptionMenu*>(c))
                {
                    genreMenu_ = m;
                    m->setListener(this);
                    m->removeAllEntry();
                    const char* n[] = {"Rock","Metal","Pop","Techno","House / EDM","Drum & Bass","Hip-Hop / Trap","R&B / Soul","Electronic","Ambient","Acoustic / Folk","Jazz","Classical","Cinematic Music","General"};
                    for (auto* x : n) m->addEntry(x);
                    m->setCurrent(static_cast<std::int32_t>(uiGenre_));
                }
                break;
            case kUiEra:
                if (auto* m = dynamic_cast<VSTGUI::COptionMenu*>(c))
                {
                    eraMenu_ = m;
                    m->setListener(this);
                    m->removeAllEntry();
                    m->addEntry("Modern");
                    m->addEntry("Vintage");
                    m->setCurrent(static_cast<std::int32_t>(uiEra_));
                }
                break;
            case kUiReset:
            case kUiAnalyze:
            case kUiDetails:
            case kUiBack:
            case kUiHelp:
            case kUiHelpClose:
                c->setListener(this);
                break;
            case kUiZoomTag:
                c->setListener(this);
                if (auto* b = dynamic_cast<VSTGUI::CTextButton*>(c))
                {
                    const auto* ae = dynamic_cast<AnalysatorEditor*>(editor);
                    setButtonTitle(b, ae && ae->isZoom100() ? "100%" : "68%");
                }
                break;
            default:
                break;
        }
    }

    refreshUi();
    return view;
}

void Controller::didOpen(VSTGUI::VST3Editor* e)
{
    editor_ = e;

    if (hasPacket_ && latestPacket_.finalState != 0 && !hasDefinitiveFinalSnapshot())
    {
        requestedFinalGeneration_ = 0;
        requestFinalSnapshot(latestPacket_.finalizationGeneration);
    }
    refreshUi();
}

void Controller::willClose(VSTGUI::VST3Editor*)
{
    clearUiPointers();
}

void Controller::viewOnEvent(VSTGUI::CView* view, VSTGUI::Event& event)
{
    if (event.type != VSTGUI::EventType::MouseDown) return;
    auto& mouse = VSTGUI::castMouseDownEvent(event);
    if (!mouse.buttonState.has(VSTGUI::MouseButton::Left)) return;

    if (view == languageLabel_)
    {
        uiLanguage_ = uiLanguage_ == Localization::Language::German ? Localization::Language::English : Localization::Language::German;
        refreshUi();
        mouse.consumed = true;
        mouse.ignoreFollowUpMoveAndUpEvents(true);
        return;
    }

    auto* c = dynamic_cast<VSTGUI::CControl*>(view);
    if (!c) return;
    switch (c->getTag())
    {
        case kUiMix:
        case kUiMaster:
        case kUiLive:
        case kUiFinal:
            break;
        default:
            return;
    }
    valueChanged(c);
    mouse.consumed = true;
    mouse.ignoreFollowUpMoveAndUpEvents(true);
}

void Controller::valueChanged(VSTGUI::CControl* c)
{
    if (!c) return;
    switch (c->getTag())
    {
        case kUiMix:
            uiMode_ = Analysis::AnalysisMode::Mix;
            break;
        case kUiMaster:
            uiMode_ = Analysis::AnalysisMode::Master;
            break;
        case kUiLive:
        case kUiAnalyze:
            if (requestLiveAnalysis() == Steinberg::kResultTrue)
            {
                uiAnalysisActive_ = true;
                uiFinalSelected_ = false;
                hasPacket_ = false;
                latestPacket_ = {};
                acceptFirstPacketAfterQueueOpen_ = true;
                requestedFinalGeneration_ = 0;
                finalSnapshotGeneration_ = 0;
            }
            break;
        case kUiFinal:
            if (uiAnalysisActive_ && requestFinalAnalysis() == Steinberg::kResultTrue)
            {
                uiAnalysisActive_ = false;
                uiFinalSelected_ = true;
            }
            break;
        case kUiGenre:
            if (genreMenu_)
            {
                const auto i = genreMenu_->getCurrentIndex();
                if (i >= 0 && i <= static_cast<std::int32_t>(Analysis::Genre::General))
                    uiGenre_ = static_cast<Analysis::Genre>(i);
            }
            break;
        case kUiEra:
            if (eraMenu_)
            {
                const auto i = eraMenu_->getCurrentIndex();
                if (i >= 0 && i <= static_cast<std::int32_t>(Analysis::Era::Vintage))
                    uiEra_ = static_cast<Analysis::Era>(i);
            }
            break;
        case kUiDetails:
        {
            uiDetailsVisible_ = true;
            uiHelpVisible_ = false;
            auto* e = editor_;
            clearUiPointers();
            editor_ = e;
            if (e)
            {
                e->exchangeView("detailsView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                {
                    ae->setNativeSize({1000., 700.});
                    ae->setUserZoom(e->getZoomFactor());
                }
            }
            return;
        }
        case kUiBack:
        {
            uiDetailsVisible_ = false;
            uiHelpVisible_ = false;
            auto* e = editor_;
            clearUiPointers();
            editor_ = e;
            if (e)
            {
                e->exchangeView("compactView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                {
                    ae->setNativeSize({650., 440.});
                    ae->setUserZoom(e->getZoomFactor());
                }
            }
            return;
        }
        case kUiHelp:
            uiHelpVisible_ = true;
            break;
        case kUiHelpClose:
            uiHelpVisible_ = false;
            break;
        case kUiReset:
            if (requestResetAnalysis() == Steinberg::kResultTrue)
            {
                uiAnalysisActive_ = false;
                uiFinalSelected_ = false;
                hasPacket_ = false;
                latestPacket_ = {};
                acceptFirstPacketAfterQueueOpen_ = true;
                requestedFinalGeneration_ = 0;
                finalSnapshotGeneration_ = 0;
            }
            break;
        case kUiZoomTag:
            if (editor_)
            {
                if (auto* e = dynamic_cast<AnalysatorEditor*>(editor_))
                {
                    const bool enlarge = !e->isZoom100();
                    const double zoom = enlarge ? kZoom100 : kZoom68;
                    if (e->setUserZoom(zoom))
                        if (auto* b = dynamic_cast<VSTGUI::CTextButton*>(c))
                            setButtonTitle(b, enlarge ? "100%" : "68%");
                }
            }
            return;
        default:
            return;
    }
    refreshUi();
}

void Controller::bindNamedView(VSTGUI::CView* view, const VSTGUI::UIAttributes& a) noexcept
{
    const auto* id = a.getAttributeValue("analysator-id");
    if (!id) return;
    if (*id == "simplePage") { simplePage_ = view; return; }
    if (*id == "detailsPage") { detailsPage_ = view; return; }
    if (*id == "helpPage") { helpPage_ = view; return; }

    if (auto* b = dynamic_cast<VSTGUI::CTextButton*>(view))
    {
        if (*id == "helpButton") { helpButton_ = b; return; }
        if (*id == "helpCloseButton") { helpCloseButton_ = b; return; }
    }

    auto* l = dynamic_cast<VSTGUI::CTextLabel*>(view);
    if (!l) return;
    if (*id == "languageLabel") { languageLabel_ = l; l->registerViewEventListener(this); }
    else if (*id == "helpTitle") helpTitle_ = l;
    else if (*id == "helpWorkflowTitle") helpWorkflowTitle_ = l;
    else if (*id == "helpWorkflowBody") helpWorkflowBody_ = l;
    else if (*id == "helpMetricsTitle") helpMetricsTitle_ = l;
    else if (*id == "helpMetricsBody") helpMetricsBody_ = l;
    else if (*id == "helpSafetyTitle") helpSafetyTitle_ = l;
    else if (*id == "helpSafetyBody") helpSafetyBody_ = l;
    else if (*id == "analysisLedGlow") analysisLedGlow_ = l;
    else if (*id == "analysisLedCore") analysisLedCore_ = l;
    else if (*id == "technicalVerdict") technicalVerdict_ = l;
    else if (*id == "styleVerdict") styleVerdict_ = l;
    else if (*id == "pcmVerdict") pcmVerdict_ = l;
    else if (*id == "streamingVerdict") streamingVerdict_ = l;
    else if (*id == "stateLabel") stateLabel_ = l;
    else if (*id == "overallVerdict") overallVerdict_ = l;
    else if (*id == "overallLine1") overallLine1_ = l;
    else if (*id == "overallLine2") overallLine2_ = l;
    else if (*id == "integratedValue") integratedValue_ = l;
    else if (*id == "truePeakValue") truePeakValue_ = l;
    else if (*id == "plrValue") plrValue_ = l;
    else if (*id == "lraValue") lraValue_ = l;
    else if (*id == "correlationValue") correlationValue_ = l;
    else if (*id == "monoValue") monoValue_ = l;
}

void Controller::updatePageVisibility() noexcept
{
    if (simplePage_) { simplePage_->setVisible(!uiDetailsVisible_); simplePage_->invalid(); }
    if (detailsPage_) { detailsPage_->setVisible(uiDetailsVisible_); detailsPage_->invalid(); }
    if (helpPage_) { helpPage_->setVisible(uiHelpVisible_); helpPage_->invalid(); }
}

void Controller::updateSelectionControls() noexcept
{
    if (mixControl_) { mixControl_->setValue(uiMode_ == Analysis::AnalysisMode::Mix ? 1.f : 0.f); mixControl_->invalid(); }
    if (masterControl_) { masterControl_->setValue(uiMode_ == Analysis::AnalysisMode::Master ? 1.f : 0.f); masterControl_->invalid(); }
    if (liveControl_) { liveControl_->setValue(uiAnalysisActive_ && !uiFinalSelected_ ? 1.f : 0.f); liveControl_->invalid(); }
    if (finalControl_) { finalControl_->setValue(uiFinalSelected_ ? 1.f : 0.f); finalControl_->invalid(); }
    if (genreMenu_) genreMenu_->setCurrent(static_cast<std::int32_t>(uiGenre_));
    if (eraMenu_) eraMenu_->setCurrent(static_cast<std::int32_t>(uiEra_));
    updatePageVisibility();
}

void Controller::refreshUi() noexcept
{
    updateSelectionControls();
    const auto text = [this](Localization::Text id) { return Localization::get(id, uiLanguage_); };

    setLabel(languageLabel_, uiLanguage_ == Localization::Language::German ? "DE" : "EN");
    setButtonTitle(helpButton_, text(Localization::Text::Help));
    setButtonTitle(helpCloseButton_, text(Localization::Text::Close));
    setLabel(helpTitle_, text(Localization::Text::Help));
    setLabel(helpWorkflowTitle_, text(Localization::Text::HelpWorkflowTitle));
    setLabel(helpWorkflowBody_, text(Localization::Text::HelpWorkflowBody));
    setLabel(helpMetricsTitle_, text(Localization::Text::HelpMetricsTitle));
    setLabel(helpMetricsBody_, text(Localization::Text::HelpMetricsBody));
    setLabel(helpSafetyTitle_, text(Localization::Text::HelpSafetyTitle));
    setLabel(helpSafetyBody_, text(Localization::Text::HelpSafetyBody));

    const bool finalPending = uiFinalSelected_ && !hasDefinitiveFinalSnapshot();
    const bool ledActive = uiAnalysisActive_ || finalPending;
    double ledLevel = ledActive ? 1.0 : 0.0;
    if (uiAnalysisActive_ && hasPacket_)
    {
        // Data exchange arrives at ~20 Hz. Ten phases therefore form a gentle
        // ~500 ms breathing cycle without a GUI timer or any audio-thread UI work.
        const auto phase = static_cast<unsigned>(latestPacket_.sequence % 10u);
        const auto triangle = phase <= 5u ? phase : 10u - phase;
        ledLevel = 0.30 + 0.70 * (static_cast<double>(triangle) / 5.0);
    }
    const auto ledColor = blendLedColor(kLedOff, kLedOn, ledLevel);
    if (analysisLedCore_)
    {
        analysisLedCore_->setFontColor(ledColor);
        analysisLedCore_->invalid();
    }
    if (analysisLedGlow_)
    {
        analysisLedGlow_->setVisible(ledActive);
        analysisLedGlow_->setAlphaValue(static_cast<float>(0.20 + 0.80 * ledLevel));
        analysisLedGlow_->invalid();
    }

    const auto a = evaluateLatest(uiMode_, uiGenre_, uiEra_);
    setVerdictLabel(technicalVerdict_, a.technicalVerdict, uiLanguage_);
    setVerdictLabel(styleVerdict_, a.styleVerdict, uiLanguage_);
    setVerdictLabel(pcmVerdict_, a.pcmDeliveryVerdict, uiLanguage_);
    setVerdictLabel(streamingVerdict_, a.streamingDeliveryVerdict, uiLanguage_);
    setVerdictLabel(overallVerdict_, a.overallVerdict, uiLanguage_);

    if (!hasPacket_)
    {
        if (uiFinalSelected_)
        {
            setColoredLabel(stateLabel_, text(Localization::Text::FinalPending), kStatePending);
            setLabel(overallLine1_, text(Localization::Text::FinalResultRequested));
            setLabel(overallLine2_, text(Localization::Text::StartPlaybackIfStopped));
        }
        else if (uiAnalysisActive_)
        {
            setColoredLabel(stateLabel_, text(Localization::Text::LiveProvisional), kStateLive);
            setLabel(overallLine1_, text(Localization::Text::PlayCompleteSong));
            setLabel(overallLine2_, text(Localization::Text::WhenFinishedFinalize));
        }
        else
        {
            setColoredLabel(stateLabel_, text(Localization::Text::Ready), kStateLive);
            setLabel(overallLine1_, text(Localization::Text::ChooseModeThenAnalyze));
            setLabel(overallLine2_, text(Localization::Text::PlayCompleteSong));
        }
        formatValue(integratedValue_, 0, "LUFS", false);
        formatValue(truePeakValue_, 0, "dBTP", false);
        formatValue(plrValue_, 0, "dB", false);
        formatValue(lraValue_, 0, "LU", false);
        formatValue(correlationValue_, 0, "", false);
        formatValue(monoValue_, 0, "dB", false);
        return;
    }

    const auto& m = latestPacket_.metrics;
    const bool fp = latestPacket_.finalState != 0;
    const bool definitive = fp && hasDefinitiveFinalSnapshot();
    const bool pending = uiFinalSelected_ && !definitive;
    if (pending)
    {
        setColoredLabel(stateLabel_, text(Localization::Text::FinalPending), kStatePending);
        setLabel(overallLine1_, text(Localization::Text::FinalResultRequested));
        setLabel(overallLine2_, text(fp ? Localization::Text::PreparingDefinitiveResult : Localization::Text::WaitingForProcessorFinalize));
    }
    else if (definitive)
    {
        setColoredLabel(stateLabel_, text(Localization::Text::FinalDefinitive), kStateFinal);
        setFinalDiagnosis(overallLine1_, overallLine2_, m, a, uiMode_, uiLanguage_);
    }
    else
    {
        setColoredLabel(stateLabel_, text(Localization::Text::LiveProvisional), kStateLive);
        setLabel(overallLine1_, text(Localization::Text::PlayCompleteSong));
        setLabel(overallLine2_, text(Localization::Text::WhenFinishedFinalize));
    }

    const bool p = m.loudnessAvailable;
    formatValue(integratedValue_, m.integratedLufs, "LUFS", p && m.integratedLufs > -999.0);
    formatValue(truePeakValue_, m.truePeakDbtp, "dBTP", p && m.truePeakDbtp > -999.0);
    formatValue(plrValue_, m.plrDb, "dB", m.plrAvailable);
    formatValue(lraValue_, m.lraLu, "LU", m.lraAvailable);
    formatValue(correlationValue_, m.correlation, "", p, 2);
    formatValue(monoValue_, m.monoCompatibilityDb, "dB", p);
}

void Controller::clearUiPointers() noexcept
{
    editor_ = nullptr;
    mixControl_ = nullptr;
    masterControl_ = nullptr;
    liveControl_ = nullptr;
    finalControl_ = nullptr;
    genreMenu_ = nullptr;
    eraMenu_ = nullptr;
    simplePage_ = nullptr;
    detailsPage_ = nullptr;
    helpPage_ = nullptr;
    languageLabel_ = nullptr;
    helpButton_ = nullptr;
    helpCloseButton_ = nullptr;
    helpTitle_ = nullptr;
    helpWorkflowTitle_ = nullptr;
    helpWorkflowBody_ = nullptr;
    helpMetricsTitle_ = nullptr;
    helpMetricsBody_ = nullptr;
    helpSafetyTitle_ = nullptr;
    helpSafetyBody_ = nullptr;
    analysisLedGlow_ = nullptr;
    analysisLedCore_ = nullptr;
    technicalVerdict_ = nullptr;
    styleVerdict_ = nullptr;
    pcmVerdict_ = nullptr;
    streamingVerdict_ = nullptr;
    stateLabel_ = nullptr;
    overallVerdict_ = nullptr;
    overallLine1_ = nullptr;
    overallLine2_ = nullptr;
    integratedValue_ = nullptr;
    truePeakValue_ = nullptr;
    plrValue_ = nullptr;
    lraValue_ = nullptr;
    correlationValue_ = nullptr;
    monoValue_ = nullptr;
}

Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage* m)
{
    if (consumeFinalSnapshotMessage(m)) return Steinberg::kResultTrue;
    if (dataExchange_.onMessage(m)) return Steinberg::kResultTrue;
    return EditController::notify(m);
}

Steinberg::tresult Controller::requestAnalysisState(Steinberg::int64 state) noexcept
{
    if (state != kAnalysisStateIdle && state != kAnalysisStateLive && state != kAnalysisStateFinal)
        return Steinberg::kInvalidArgument;
    auto* m = allocateMessage();
    if (!m) return Steinberg::kOutOfMemory;
    m->setMessageID(kSetAnalysisStateMessage);
    Steinberg::tresult r = Steinberg::kResultFalse;
    if (auto* a = m->getAttributes())
        if (a->setInt(kAnalysisStateKey, state) == Steinberg::kResultTrue)
            r = sendMessage(m);
    m->release();
    return r;
}

Steinberg::tresult Controller::requestResetAnalysis() noexcept { return requestAnalysisState(kAnalysisStateIdle); }
Steinberg::tresult Controller::requestLiveAnalysis() noexcept { return requestAnalysisState(kAnalysisStateLive); }
Steinberg::tresult Controller::requestFinalAnalysis() noexcept { return requestAnalysisState(kAnalysisStateFinal); }

bool Controller::consumeFinalSnapshotMessage(Steinberg::Vst::IMessage* m) noexcept
{
    if (!m || !m->getMessageID() || std::strcmp(m->getMessageID(), kFinalSnapshotMessage) != 0) return false;
    auto* a = m->getAttributes();
    if (!a) return true;
    Steinberg::int64 gv = 0;
    const void* data = nullptr;
    Steinberg::uint32 size = 0;
    if (a->getInt(kFinalSnapshotGenerationKey, gv) != Steinberg::kResultTrue ||
        a->getBinary(kFinalSnapshotDataKey, data, size) != Steinberg::kResultTrue ||
        gv < 0 || !data || size != sizeof(DSP::AnalysisSnapshot))
    {
        requestedFinalGeneration_ = 0;
        return true;
    }
    const auto g = static_cast<std::uint64_t>(gv);
    if (!hasPacket_ || latestPacket_.finalState == 0 || latestPacket_.finalizationGeneration != g)
    {
        requestedFinalGeneration_ = 0;
        return true;
    }
    DSP::AnalysisSnapshot s;
    std::memcpy(&s, data, sizeof(s));
    if (!s.valid)
    {
        requestedFinalGeneration_ = 0;
        return true;
    }
    latestPacket_.metrics = Analysis::AssessmentInput::fromFinal(s);
    finalSnapshotGeneration_ = g;
    refreshUi();
    return true;
}

void Controller::requestFinalSnapshot(std::uint64_t g) noexcept
{
    if (g == 0 || g == requestedFinalGeneration_) return;
    auto* m = allocateMessage();
    if (!m) return;
    m->setMessageID(kRequestFinalSnapshotMessage);
    if (auto* a = m->getAttributes())
    {
        a->setInt(kFinalSnapshotGenerationKey, static_cast<Steinberg::int64>(g));
        if (sendMessage(m) == Steinberg::kResultTrue) requestedFinalGeneration_ = g;
    }
    m->release();
}

void PLUGIN_API Controller::queueOpened(Steinberg::Vst::DataExchangeUserContextID id,
                                        Steinberg::uint32 size,
                                        Steinberg::TBool& bg)
{
    if (id != kAnalysisExchangeContext || size < sizeof(AnalysisExchangePacket)) return;
    bg = false;
    acceptFirstPacketAfterQueueOpen_ = true;
    if (hasPacket_ && latestPacket_.finalState != 0 && !hasDefinitiveFinalSnapshot())
    {
        requestedFinalGeneration_ = 0;
        requestFinalSnapshot(latestPacket_.finalizationGeneration);
    }
}

void PLUGIN_API Controller::queueClosed(Steinberg::Vst::DataExchangeUserContextID id)
{
    if (id == kAnalysisExchangeContext) acceptFirstPacketAfterQueueOpen_ = true;
}

void PLUGIN_API Controller::onDataExchangeBlocksReceived(Steinberg::Vst::DataExchangeUserContextID id,
                                                          Steinberg::uint32 n,
                                                          Steinberg::Vst::DataExchangeBlock* blocks,
                                                          Steinberg::TBool)
{
    if (id != kAnalysisExchangeContext || !blocks) return;
    bool changed = false;
    for (Steinberg::uint32 i = 0; i < n; ++i)
    {
        if (!blocks[i].data || blocks[i].size < sizeof(AnalysisExchangePacket)) continue;
        AnalysisExchangePacket p;
        std::memcpy(&p, blocks[i].data, sizeof(p));
        const bool sequenceEpochStart = acceptFirstPacketAfterQueueOpen_;
        if (!sequenceEpochStart && hasPacket_ && p.sequence < latestPacket_.sequence) continue;
        acceptFirstPacketAfterQueueOpen_ = false;
        const bool preserveLastValidMetrics = hasPacket_ && latestPacket_.metrics.loudnessAvailable && !p.metrics.loudnessAvailable;
        if (preserveLastValidMetrics)
        {
            const auto previousMetrics = latestPacket_.metrics;
            latestPacket_ = p;
            latestPacket_.metrics = previousMetrics;
        }
        else
        {
            latestPacket_ = p;
        }
        hasPacket_ = true;
        changed = true;
        if (p.finalState != 0)
        {
            uiAnalysisActive_ = false;
            uiFinalSelected_ = true;
            requestFinalSnapshot(p.finalizationGeneration);
        }
        else
        {
            uiAnalysisActive_ = true;
            uiFinalSelected_ = false;
            requestedFinalGeneration_ = 0;
            finalSnapshotGeneration_ = 0;
        }
    }
    if (changed) refreshUi();
}

Analysis::Assessment Controller::evaluateLatest(Analysis::AnalysisMode mode,
                                                Analysis::Genre genre,
                                                Analysis::Era era) const noexcept
{
    if (!hasPacket_)
    {
        Analysis::Metrics u;
        u.loudnessAvailable = false;
        u.plrAvailable = false;
        u.lraAvailable = false;
        u.provisional = true;
        return Analysis::AssessmentModel::evaluate(u, mode, genre, era);
    }
    if (latestPacket_.finalState != 0 && !hasDefinitiveFinalSnapshot())
    {
        auto w = latestPacket_.metrics;
        w.provisional = true;
        return Analysis::AssessmentModel::evaluate(w, mode, genre, era);
    }
    return Analysis::AssessmentModel::evaluate(latestPacket_.metrics, mode, genre, era);
}
}
