from pathlib import Path
import subprocess

p = Path('src/plugin/AnalysatorController.cpp')
s = p.read_text(encoding='utf-8')
old = '''class AnalysatorEditor final : public VSTGUI::VST3Editor
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
};'''
new = '''class AnalysatorEditor final : public VSTGUI::VST3Editor
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

    bool isZoom100() const noexcept { return getZoomFactor() > 1.25; }

    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description) override;

private:
    VSTGUI::CPoint nativeSize_;
};'''
if old not in s:
    raise SystemExit('AnalysatorEditor block not found')
s = s.replace(old, new)
old2 = '''        const char* viewName = uiDetailsVisible_ ? "detailsView" : "compactView";
        auto* e = new AnalysatorEditor(this, viewName, "analysator.uidesc");
        e->setDelegate(this);
        e->setZoomFactor(kZoom68);
        e->setAllowedZoomFactors({kZoom68, kZoom100});
        return e;'''
new2 = '''        const char* viewName = uiDetailsVisible_ ? "detailsView" : "compactView";
        const VSTGUI::CPoint nativeSize = uiDetailsVisible_ ? VSTGUI::CPoint{1000., 700.}
                                                            : VSTGUI::CPoint{650., 440.};
        auto* e = new AnalysatorEditor(this, viewName, "analysator.uidesc", nativeSize);
        e->setDelegate(this);
        e->setZoomFactor(kZoom68);
        e->setAllowedZoomFactors({kZoom68, kZoom100});
        e->setEditorSizeConstrains(nativeSize, nativeSize);
        return e;'''
if old2 not in s:
    raise SystemExit('createView block not found')
s = s.replace(old2, new2)
p.write_text(s, encoding='utf-8')
subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp'], check=True)
subprocess.run(['git','commit','-m','Force host resize when returning to 100 percent'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
