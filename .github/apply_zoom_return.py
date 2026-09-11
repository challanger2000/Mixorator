from pathlib import Path
import subprocess

cpp = Path('src/plugin/AnalysatorController.cpp')
hdr = Path('src/plugin/AnalysatorController.h')
s = cpp.read_text(encoding='utf-8')
h = hdr.read_text(encoding='utf-8')

old = '''    AnalysatorEditor(Steinberg::Vst::EditController* controller,
                     const char* viewName,
                     const char* xmlFile,
                     const VSTGUI::CPoint& nativeSize)
    : VSTGUI::VST3Editor(controller, viewName, xmlFile), nativeSize_(nativeSize) {}
'''
new = '''    AnalysatorEditor(Steinberg::Vst::EditController* controller,
                     const char* viewName,
                     const char* xmlFile,
                     const VSTGUI::CPoint& nativeSize,
                     double* persistedZoom)
    : VSTGUI::VST3Editor(controller, viewName, xmlFile),
      nativeSize_(nativeSize), persistedZoom_(persistedZoom) {}
'''
if old not in s:
    raise SystemExit('editor constructor block not found')
s = s.replace(old, new, 1)

old = '''        setZoomFactor(factor);
        setEditorSizeConstrains(nativeSize_, nativeSize_);
        return true;
'''
new = '''        setZoomFactor(factor);
        if (persistedZoom_)
            *persistedZoom_ = factor;
        setEditorSizeConstrains(nativeSize_, nativeSize_);
        return true;
'''
if old not in s:
    raise SystemExit('setUserZoom block not found')
s = s.replace(old, new, 1)

old = '''private:
    VSTGUI::CPoint nativeSize_;
};
'''
new = '''private:
    VSTGUI::CPoint nativeSize_;
    double* persistedZoom_ {};
};
'''
if old not in s:
    raise SystemExit('editor private block not found')
s = s.replace(old, new, 1)

old = '''        auto* e = new AnalysatorEditor(this, viewName, "analysator.uidesc", nativeSize);
        e->setDelegate(this);
        e->setZoomFactor(kZoom68);
        e->setAllowedZoomFactors({kZoom68, kZoom100});
        e->setEditorSizeConstrains(nativeSize, nativeSize);
'''
new = '''        auto* e = new AnalysatorEditor(this, viewName, "analysator.uidesc", nativeSize, &uiZoomFactor_);
        e->setDelegate(this);
        e->setZoomFactor(uiZoomFactor_);
        e->setAllowedZoomFactors({kZoom68, kZoom100});
        e->setEditorSizeConstrains(nativeSize, nativeSize);
'''
if old not in s:
    raise SystemExit('createView zoom block not found')
s = s.replace(old, new, 1)

old = '''        ae->setNativeSize(nativeSize);
        ae->setUserZoom(kZoom68);
'''
new = '''        ae->setNativeSize(nativeSize);
        ae->setUserZoom(uiZoomFactor_);
'''
if old not in s:
    raise SystemExit('didOpen zoom restore block not found')
s = s.replace(old, new, 1)

oldh = '''    bool uiDetailsVisible_ {false};
    bool uiHelpVisible_ {false};

    VSTGUI::VST3Editor* editor_ {nullptr};
'''
newh = '''    bool uiDetailsVisible_ {false};
    bool uiHelpVisible_ {false};
    double uiZoomFactor_ {1.0};

    VSTGUI::VST3Editor* editor_ {nullptr};
'''
if oldh not in h:
    raise SystemExit('controller zoom state insertion point not found')
h = h.replace(oldh, newh, 1)

cpp.write_text(s, encoding='utf-8')
hdr.write_text(h, encoding='utf-8')
subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp','src/plugin/AnalysatorController.h'], check=True)
subprocess.run(['git','commit','-m','Preserve zoom state when editor reopens'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
