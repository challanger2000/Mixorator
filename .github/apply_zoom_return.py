from pathlib import Path
import subprocess

p = Path('src/plugin/AnalysatorController.cpp')
s = p.read_text(encoding='utf-8')

old = '''void Controller::didOpen(VSTGUI::VST3Editor* e)
{
    editor_ = e;

    if (hasPacket_ && latestPacket_.finalState != 0 && !hasDefinitiveFinalSnapshot())
'''
new = '''void Controller::didOpen(VSTGUI::VST3Editor* e)
{
    editor_ = e;

    // Studio One can keep the previous outer plug-in frame size while the editor
    // is hidden.  createView() sets constraints before the host frame is attached,
    // so re-apply the active page's native size here, after didOpen, and force the
    // editor back to the native 100% state.  This keeps content and host frame in
    // sync when the plug-in is hidden and shown again.
    if (auto* ae = dynamic_cast<AnalysatorEditor*>(e))
    {
        const VSTGUI::CPoint nativeSize = uiDetailsVisible_ ? VSTGUI::CPoint{1000., 700.}
                                                            : VSTGUI::CPoint{650., 440.};
        ae->setNativeSize(nativeSize);
        ae->setUserZoom(kZoom68);
    }

    if (hasPacket_ && latestPacket_.finalState != 0 && !hasDefinitiveFinalSnapshot())
'''
if old not in s:
    raise SystemExit('didOpen block not found')
s = s.replace(old, new, 1)

p.write_text(s, encoding='utf-8')
subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp'], check=True)
subprocess.run(['git','commit','-m','Sync host frame when editor reopens'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
