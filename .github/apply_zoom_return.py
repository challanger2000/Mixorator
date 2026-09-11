from pathlib import Path
import subprocess

ui = Path('src/resource/analysator.uidesc')
u = ui.read_text(encoding='utf-8')

old = '<view class="CViewContainer" origin="24, 397" size="64, 30" transparent="true" bitmap="Brand125AMain"/>'
new = '<view class="CViewContainer" origin="24, 397" size="64, 30" transparent="false" bitmap="Brand125AMain"/>'

if old not in u:
    raise SystemExit('logo view block not found')

u = u.replace(old, new, 1)
ui.write_text(u, encoding='utf-8')

subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/resource/analysator.uidesc'], check=True)
subprocess.run(['git','commit','-m','Fix compact 125A logo rendering'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
