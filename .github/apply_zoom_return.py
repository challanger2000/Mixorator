from pathlib import Path
import subprocess

p = Path('src/plugin/AnalysatorController.cpp')
s = p.read_text(encoding='utf-8')

old = '''    bool isZoom100() const noexcept { return getZoomFactor() > 1.25; }

    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
'''
new = '''    void setNativeSize(const VSTGUI::CPoint& nativeSize)
    {
        nativeSize_ = nativeSize;
        setEditorSizeConstrains(nativeSize_, nativeSize_);
    }

    bool isZoom100() const noexcept { return getZoomFactor() > 1.25; }

    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
'''
if old not in s:
    raise SystemExit('AnalysatorEditor insertion point not found')
s = s.replace(old, new, 1)

old_details = '''                e->exchangeView("detailsView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                    ae->setUserZoom(e->getZoomFactor());
'''
new_details = '''                e->exchangeView("detailsView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                {
                    ae->setNativeSize({1000., 700.});
                    ae->setUserZoom(e->getZoomFactor());
                }
'''
if old_details not in s:
    raise SystemExit('details exchange block not found')
s = s.replace(old_details, new_details, 1)

old_back = '''                e->exchangeView("compactView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                    ae->setUserZoom(e->getZoomFactor());
'''
new_back = '''                e->exchangeView("compactView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                {
                    ae->setNativeSize({650., 440.});
                    ae->setUserZoom(e->getZoomFactor());
                }
'''
if old_back not in s:
    raise SystemExit('compact exchange block not found')
s = s.replace(old_back, new_back, 1)

p.write_text(s, encoding='utf-8')
subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp'], check=True)
subprocess.run(['git','commit','-m','Fix details zoom native size constraints'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
