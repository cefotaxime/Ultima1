#!/usr/bin/env python3
from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
build=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=231
assert (('bool hasFutureVoice=false;' in sfx and 'if(!hasFutureVoice) return;' in sfx) or ('app-queue=none' in sfx))
for q in ['U1_SFX_TRACE','[SFXTRACE]','play_begin id=','play_end id=']:
    assert q in sfx,q
assert ('queued_bytes=' in sfx) or ('app_queue_samples=0' in sfx)
for q in ['pactl info','pactl list sinks','Latency:','U1_SFX_TRACE=1']:
    assert q in build,q
assert 'SOURCE_US' in pc and '4302' in pc
if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert out[0x3119:0x3120]==bytes.fromhex('b8 04 00 50 e8 56 d5')
print('M231 no-idle-prefill / host-audio instrumentation audit OK')
