from pathlib import Path
import subprocess

ui = Path('src/resource/analysator.uidesc')
u = ui.read_text(encoding='utf-8')

# Use the proven 125A technique from LFOator/LightOrgan: two text labels,
# not a bitmap/container. Keep it only in the compact/main view.
u = u.replace('<bitmap name="Brand125AMain" path="125a_main.png"/>', '')
old = '<view class="CViewContainer" origin="24, 397" size="64, 30" transparent="false" bitmap="Brand125AMain"/>'
new = '''<view class="CTextLabel" origin="24, 398" size="39, 24" title="125" font-color="Text" font="StatusFont" text-alignment="center" transparent="true"/>
      <view class="CTextLabel" origin="60, 398" size="17, 24" title="A" font-color="SignetRed" font="StatusFont" text-alignment="center" transparent="true"/>'''
if old not in u:
    raise SystemExit('old bitmap logo view not found')
u = u.replace(old, new, 1)

if 'name="SignetRed"' not in u:
    u = u.replace('<color name="LedGlow" rgba="#D9342F55"/>', '<color name="LedGlow" rgba="#D9342F55"/><color name="SignetRed" rgba="#D22F2FFF"/>')

if 'title="125"' not in u or 'font-color="SignetRed"' not in u:
    raise SystemExit('text signet replacement failed')

ui.write_text(u, encoding='utf-8')
subprocess.run(['git','diff','--check'], check=True)
subprocess.run(['git','config','user.name','github-actions[bot]'], check=True)
subprocess.run(['git','config','user.email','41898282+github-actions[bot]@users.noreply.github.com'], check=True)
subprocess.run(['git','add','src/resource/analysator.uidesc'], check=True)
subprocess.run(['git','commit','-m','Use proven text-based 125A signet'], check=True)
subprocess.run(['git','push','origin','HEAD:gui-foundation'], check=True)
