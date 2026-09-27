#!/usr/bin/env python3
from pathlib import Path
import re, sys, wave

root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
sfxh=(root/'src/SfxSystem.h').read_text(encoding='utf-8')

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=227
assert 'SDL_QueueAudio' not in sfx
assert 'SDL_ClearQueuedAudio' not in sfx
for q in [
    'want.callback=&SfxSystem::audioCallback;',
    'want.samples=64;',
    'voices_.push_back(Voice{clip.data,clip.samples,startSample});',
    'mixed+=v.data',
    'sourceCursorSample_=startSample+logicalSamples;',
    'const std::uint64_t startSample=sourceCursorSample_;',
]: assert q in sfx,q
assert 'std::vector<Voice> voices_' in sfxh
assert 'bool logicalBusy() const;' in sfxh

fn=root/'data'/'sfx_capture_oracle_m225'/'sound_04.wav'
with wave.open(str(fn),'rb') as w:
    pcm_ms=w.getnframes()*1000.0/w.getframerate()
assert pcm_ms>20.0
assert '5,  // 4 movement' in sfx or '48,51,78,99,5,900' in sfx

for q in [
    'if(noiseEnabled) sfx.playImmediate(SfxEvent::LocalPlayerMove);',
    'for(int i=0;i<localMoveSounds;++i) sfx.play(SfxEvent::LocalActorMove);',
    'bool keyDispatchedThisFrame=false;',
    'while(!sourceSfxBusy && !keyDispatchedThisFrame && SDL_PollEvent(&e)){',
    'const bool sourceSfxBusy=noiseEnabled && sfx.logicalBusy();',
]: assert q in main,q

if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    for off,b in {
        0x08CE: bytes.fromhex('b8 04 00 50 e8 a1 fd'),
        0x27A4: bytes.fromhex('b8 04 00 50 e8 cb de'),
        0x28D4: bytes.fromhex('b8 04 00 50 e8 9b dd'),
        0x2A5D: bytes.fromhex('b8 04 00 50 e8 12 dc'),
        0x2B36: bytes.fromhex('b8 04 00 50 e8 39 db'),
        0x3119: bytes.fromhex('b8 04 00 50 e8 56 d5'),
    }.items(): assert out[off:off+len(b)]==b

print('M227 source-timed overlapping SFX / one-command-per-present audit OK')
