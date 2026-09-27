#!/usr/bin/env python3
from pathlib import Path
import re,sys,wave,array
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=234
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=234

for q in ['want.callback=&SfxSystem::audioCallback;','want.userdata=this;','transport=SDL-direct-callback','app-queue=none','renderCallback(','SDL_LockAudioDevice(device_)','waitSourceDurationUs(us);']:
    assert q in sfx,q
for q in ['SDL_QueueAudio(','SDL_GetQueuedAudioSize(','SDL_CreateThread(&SfxSystem::feederEntry','pumpLocked()','playedSamplesLocked()']:
    assert q not in sfx,q

assert 'SfxPcmOracleM225' not in sfx
for q in ['IMPULSE_QUALITY=150','IMPULSE_OS=32','SINC_FADE=0.999','makeImpulseLut()','makeFirstOrderHighPass(120.0)','makeSecondOrderHighPass(120.0,1.0)','makeFirstOrderLowPass(4300.0)','makeSecondOrderLowPass(4300.0,1.0)','SOUND4_INTERVALS_48K','12, 21, 21, 21, 22, 22, 22, 22']:
    assert q in pc,q

p=root/'data/sfx_capture_oracle_m225/sound_04.wav'
with wave.open(str(p),'rb') as w:
    assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
    raw=w.readframes(w.getnframes())
    x=array.array('h'); x.frombytes(raw)
    assert max(abs(v) for v in x) >= 8000

if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert out[0x3119:0x3120]==bytes.fromhex('b8 04 00 50 e8 56 d5')
print('M234 direct-callback / DOSBox-style impulse-model audit OK')
