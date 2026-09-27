#!/usr/bin/env python3
from pathlib import Path
import re, wave, array
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=232
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=232
assert ('if(!hasFutureVoice) return;' in sfx) or ('app-queue=none' in sfx)
# M232 pitch correction remains valid; its aggressive transport experiment was superseded.
for q in ['SOUND4_INTERVALS_48K','12,21,21,21,22,22,22,22','4302','SOUND4_INTERVALS_48K']:
    assert q in pc,q
p=root/'data/sfx_capture_oracle_m225/sound_04.wav'
with wave.open(str(p),'rb') as w:
    raw=w.readframes(w.getnframes())
    x=array.array('h'); x.frombytes(raw)
    onset=next((i for i,v in enumerate(x) if abs(v)>100),-1)
    assert 8 <= onset <= 16,onset
intervals=[21,21,21,22,22,22,22]
avg=sum(intervals)/len(intervals)
f0=48000.0/(2.0*avg)
assert 1080.0 < f0 < 1180.0,f0
print('M232 sound4 pitch calibration compatibility audit OK')
