from pathlib import Path
import subprocess

REPO_BASE = 'fcb25ee21197221c61a086a4c4d6efef7e080cd4'

def run(*args):
    subprocess.run(args, check=True)

# Return the visual assets and authored GUI to the last known native 650x440 / 1000x700 state.
run('git','checkout',REPO_BASE,'--','src/resource/analysator.uidesc','src/resource/analysator_main_bg.png','src/resource/analysator_details_bg.png')

p = Path('src/plugin/AnalysatorController.cpp')
s = p.read_text(encoding='utf-8')

if '#include "vstgui/lib/cdrawcontext.h"' not in s:
    s = s.replace('#include "vstgui/lib/ccolor.h"\n', '#include "vstgui/lib/ccolor.h"\n#include "vstgui/lib/cdrawcontext.h"\n#include "vstgui/lib/cfont.h"\n#include "vstgui/uidescription/uiattributes.h"\n')

old = '''constexpr double kZoom68 = 0.68;
constexpr double kZoom100 = 1.0;
constexpr std::int32_t kUiZoomTag = 10013;

class AnalysatorEditor final : public VSTGUI::VST3Editor
{
public:
    using VSTGUI::VST3Editor::VST3Editor;

    bool setUserZoom(double factor)
    {
        if (factor != kZoom68 && factor != kZoom100)
            return false;
        setZoomFactor(factor);
        return true;
    }

    bool isZoom100() const noexcept { return getZoomFactor() > 0.84; }
};'''
new = '''constexpr double kZoom68 = 1.0;   // native GUI = 100%
constexpr double kZoom100 = 1.5;  // enlarged GUI = 150%
constexpr std::int32_t kUiZoomTag = 10013;

class AnalysatorEditor final : public VSTGUI::VST3Editor
{
public:
    using VSTGUI::VST3Editor::VST3Editor;

    bool setUserZoom(double factor)
    {
        if (factor != kZoom68 && factor != kZoom100)
            return false;
        setZoomFactor(factor);
        return true;
    }

    bool isZoom100() const noexcept { return getZoomFactor() > 1.25; }

    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description) override;
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
}'''
if old not in s:
    raise SystemExit('Expected current AnalysatorEditor block not found')
s = s.replace(old,new)
p.write_text(s,encoding='utf-8')

u = Path('src/resource/analysator.uidesc')
t = u.read_text(encoding='utf-8')
compact_old = '<view class="CTextButton" control-tag="UI.Zoom" origin="478, 12" size="62, 20" title="68%" text-color="MutedText" text-color-highlighted="Text" font="SmallFont" text-alignment="center" frame-width="0" gradient="HardwareClear" gradient-highlighted="HardwareAction" kick-style="true"/>'
details_old = '<view class="CTextButton" control-tag="UI.Zoom" origin="914, 6" size="66, 18" title="68%" text-color="MutedText" text-color-highlighted="Text" font="SmallFont" text-alignment="center" frame-width="0" gradient="HardwareClear" gradient-highlighted="HardwareAction" kick-style="true"/>'
if compact_old not in t or details_old not in t:
    raise SystemExit('Expected original zoom controls not found')
t = t.replace(compact_old,'<view class="CView" origin="478, 12" size="62, 20" custom-view-name="UiZoomCompact" transparent="true"/>')
t = t.replace(details_old,'<view class="CView" origin="914, 6" size="66, 18" custom-view-name="UiZoomDetails" transparent="true"/>')
u.write_text(t,encoding='utf-8')

# Verify intended state before committing.
assert 'size="650, 440"' in t
assert 'size="1000, 700"' in t
assert 'Brand125A' not in t
assert 'constexpr double kZoom68 = 1.0;' in s
assert 'constexpr double kZoom100 = 1.5;' in s
assert 'editor_->setUserZoom' in s

# Restore normal workflows, delete this one-shot helper and script.
run('git','checkout',REPO_BASE,'--','.github/workflows/build-windows.yml','.github/workflows/finalize-analysator-name.yml')
Path('.github/apply_direct_zoom.py').unlink(missing_ok=True)
Path('.github/workflows/fix-zoom-direct.yml').unlink(missing_ok=True)

run('git','config','user.name','github-actions[bot]')
run('git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com')
run('git','add','-A')
run('git','commit','-m','Use proven direct editor zoom at 100 and 150 percent')
run('git','push','origin','HEAD:gui-foundation')
