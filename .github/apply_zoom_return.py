from pathlib import Path
import re
import subprocess
from PIL import Image

cpp = Path('src/plugin/AnalysatorController.cpp')
hdr = Path('src/plugin/AnalysatorController.h')
ui = Path('src/resource/analysator.uidesc')

s = cpp.read_text(encoding='utf-8')
h = hdr.read_text(encoding='utf-8')
u = ui.read_text(encoding='utf-8')

# Remove all custom zoom/editor machinery and return to a plain fixed-size VST3 editor.
s, n = re.subn(
    r'constexpr double kZoom68 = 1\.0;.*?VSTGUI::CView\* AnalysatorEditor::createView\(.*?\n\}\n(?=// Verdict palette)',
    '', s, count=1, flags=re.S)
if n != 1:
    raise SystemExit('zoom class block not found')

old = '''        const char* viewName = uiDetailsVisible_ ? "detailsView" : "compactView";
        const VSTGUI::CPoint nativeSize = uiDetailsVisible_ ? VSTGUI::CPoint{1000., 700.}
                                                            : VSTGUI::CPoint{650., 440.};
        auto* e = new AnalysatorEditor(this, viewName, "analysator.uidesc", nativeSize, &uiZoomFactor_);
        e->setDelegate(this);
        e->setZoomFactor(uiZoomFactor_);
        e->setAllowedZoomFactors({kZoom68, kZoom100});
        e->setEditorSizeConstrains(nativeSize, nativeSize);
        return e;
'''
new = '''        const char* viewName = uiDetailsVisible_ ? "detailsView" : "compactView";
        auto* e = new VSTGUI::VST3Editor(this, viewName, "analysator.uidesc");
        e->setDelegate(this);
        return e;
'''
if old not in s:
    raise SystemExit('createView block not found')
s = s.replace(old, new, 1)

s, n = re.subn(
    r'\n\s*case kUiZoomTag:\n\s*c->setListener\(this\);\n\s*if \(auto\* b = dynamic_cast<VSTGUI::CTextButton\*>\(c\)\)\n\s*\{.*?\n\s*\}\n\s*break;',
    '', s, count=1, flags=re.S)
if n != 1:
    raise SystemExit('verifyView zoom case not found')

s, n = re.subn(
    r'\n\s*// Studio One can keep the previous outer plug-in frame size.*?ae->setUserZoom\(uiZoomFactor_\);\n\s*\}\n',
    '\n', s, count=1, flags=re.S)
if n != 1:
    raise SystemExit('didOpen zoom sync block not found')

old = '''            if (e)
            {
                e->exchangeView("detailsView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                {
                    ae->setNativeSize({1000., 700.});
                    ae->setUserZoom(e->getZoomFactor());
                }
            }
'''
new = '''            if (e)
                e->exchangeView("detailsView");
'''
if old not in s:
    raise SystemExit('details exchange block not found')
s = s.replace(old, new, 1)

old = '''            if (e)
            {
                e->exchangeView("compactView");
                if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
                {
                    ae->setNativeSize({650., 440.});
                    ae->setUserZoom(e->getZoomFactor());
                }
            }
'''
new = '''            if (e)
                e->exchangeView("compactView");
'''
if old not in s:
    raise SystemExit('compact exchange block not found')
s = s.replace(old, new, 1)

s, n = re.subn(
    r'\n\s*case kUiZoomTag:\n\s*if \(editor_\).*?\n\s*return;',
    '', s, count=1, flags=re.S)
if n != 1:
    raise SystemExit('valueChanged zoom case not found')

h = h.replace('    double uiZoomFactor_ {1.0};\n', '')

# Remove zoom controls/tags and add one physically scaled 125A logo at bottom-left of Main.
u = u.replace('<bitmap name="AnalysatorDetailsBg" path="analysator_details_bg.png"/>',
              '<bitmap name="AnalysatorDetailsBg" path="analysator_details_bg.png"/><bitmap name="Brand125AMain" path="125a_main.png"/>')
u = u.replace('<control-tag name="UI.Zoom" tag="10013"/>', '')
u = re.sub(r'<view class="CView" origin="478, 12" size="62, 20" custom-view-name="UiZoomCompact" transparent="true"/>', '', u)
u = re.sub(r'<view class="CView" origin="914, 6" size="66, 18" custom-view-name="UiZoomDetails" transparent="true"/>', '', u)
anchor = '<view class="CTextLabel" origin="110, 400" size="68, 15" title="GENRE"'
logo = '<view class="CViewContainer" origin="24, 397" size="64, 30" transparent="true" bitmap="Brand125AMain"/>\n      '
if anchor not in u:
    raise SystemExit('compact footer anchor not found')
u = u.replace(anchor, logo + anchor, 1)

# Make the bitmap itself 64x30 so VSTGUI cannot draw the original 132x62 image oversized.
src = Image.open('src/resource/125a_gui.png').convert('RGBA')
src.resize((64, 30), Image.Resampling.LANCZOS).save('src/resource/125a_main.png')

cpp.write_text(s, encoding='utf-8')
hdr.write_text(h, encoding='utf-8')
ui.write_text(u, encoding='utf-8')

subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp','src/plugin/AnalysatorController.h','src/resource/analysator.uidesc','src/resource/125a_main.png'], check=True)
subprocess.run(['git','commit','-m','Remove zoom and add compact 125A logo'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
