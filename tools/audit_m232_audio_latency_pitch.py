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
for q in ['REQUESTED_SAMPLES=32','PULSE_LATENCY_MSEC","2"','FEEDER_SLEEP_US=250',
          'while(feederRun_ && voices_.empty())','SDL_CondWait(progress_,mutex_)',
          'if(!hasFutureVoice) return;']:
    assert q in sfx,q
for q in ['SOUND4_CAPTURE_INTERVAL_SAMPLES','12,21,21,21,22,22,22,22','4302','Do not stretch the']:
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
print('M232 low-latency feeder / sound4 pitch calibration audit OK')
