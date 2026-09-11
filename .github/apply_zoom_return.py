from pathlib import Path
import subprocess

cpp = Path('src/plugin/AnalysatorController.cpp')
s = cpp.read_text(encoding='utf-8')

old_details = '''            if (e)
                e->exchangeView("detailsView");
            return;
'''
new_details = '''            if (e)
            {
                e->exchangeView("detailsView");
                e->setEditorSizeConstrains({1000., 700.}, {1000., 700.});
            }
            return;
'''
old_back = '''            if (e)
                e->exchangeView("compactView");
            return;
'''
new_back = '''            if (e)
            {
                e->exchangeView("compactView");
                e->setEditorSizeConstrains({650., 440.}, {650., 440.});
            }
            return;
'''

if old_details not in s:
    raise SystemExit('details exchange block not found')
if old_back not in s:
    raise SystemExit('compact exchange block not found')

s = s.replace(old_details, new_details, 1)
s = s.replace(old_back, new_back, 1)
cpp.write_text(s, encoding='utf-8')

subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/plugin/AnalysatorController.cpp'], check=True)
subprocess.run(['git','commit','-m','Resize host when switching fixed GUI views'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
