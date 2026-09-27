#!/usr/bin/env python3
from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
notes=(root/'ORACLE_NOTES_M233.md').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=233
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=233
for q in ['FEEDER_SLEEP_MS=1','REQUESTED_SAMPLES=64','PULSE_LATENCY_MSEC","5"','SDL_Delay(FEEDER_SLEEP_MS)','if(!hasFutureVoice) return;','idle-silence-prefill=off, transport=M231-stable']:
    assert q in sfx,q
for q in ['FEEDER_SLEEP_US=250','REQUESTED_SAMPLES=32','PULSE_LATENCY_MSEC","2"','std::this_thread::sleep_for','while(feederRun_ && voices_.empty())']:
    assert q not in sfx,q
for q in ['SOUND4_CAPTURE_INTERVAL_SAMPLES','12, 21, 21, 21, 22, 22, 22, 22','Do not stretch the']:
    assert q in pc,q
assert '7.5 steps' in notes and 'M231' in notes
if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert out[0x3119:0x3120]==bytes.fromhex('b8 04 00 50 e8 56 d5')
print('M233 M231-transport + M232-footstep-pitch audit OK')
