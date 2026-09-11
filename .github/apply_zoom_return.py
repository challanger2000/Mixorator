from pathlib import Path
import subprocess

cpp = Path('src/plugin/AnalysatorController.cpp')
s = cpp.read_text(encoding='utf-8')

old = '''        else
        {
            uiAnalysisActive_ = true;
            uiFinalSelected_ = false;
            requestedFinalGeneration_ = 0;
            finalSnapshotGeneration_ = 0;
        }
'''
new = '''        else if (!uiFinalSelected_)
        {
            // A stale live packet may arrive after the user pressed FINAL.
            // Do not let it switch the UI back to LIVE/VORLAEUFIG while
            // the definitive final snapshot is being requested.
            uiAnalysisActive_ = true;
            uiFinalSelected_ = false;
            requestedFinalGeneration_ = 0;
            finalSnapshotGeneration_ = 0;
        }
'''

if old not in s:
    raise SystemExit('live packet state block not found')

s = s.replace(old, new, 1)
cpp.write_text(s, encoding='utf-8')

subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp'], check=True)
subprocess.run(['git','commit','-m','Keep final state while stale live packets drain'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
