#!/usr/bin/env python3
from pathlib import Path
import re,sys,wave
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
sfxh=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
notes=(root/'ORACLE_NOTES_M229.md').read_text(encoding='utf-8')

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=229
for q in ['want.samples=64;','want.callback=nullptr;','SDL_CreateThread(&SfxSystem::feederEntry',
          'SDL_QueueAudio(device_,chunk.data()','constexpr std::uint32_t FEEDER_SLEEP_MS = 1;',
          'constexpr std::uint32_t MIN_LEAD_MS = 4;','const std::uint64_t targetLead=std::max(minLead,devicePeriod);']:
    assert q in sfx,q
sched=sfx[sfx.index('void SfxSystem::schedule'):sfx.index('bool SfxSystem::logicalBusy')]
assert 'SDL_ClearQueuedAudio' not in sched
assert 'SDL_QueueAudio' not in sched
assert 'voices_.push_back(Voice{clip.data,clip.samples,startSample})' in sched
assert 'SDL_QueueAudio(device_,clip.data' not in sfx
assert 'Voice {' in sfxh and 'sourceCursorSample_' in sfxh
for q in ['bool keyDispatchedThisFrame=false;',
          'while(!sourceSfxBusy && !keyDispatchedThisFrame && SDL_PollEvent(&e)){',
          'const bool sourceSfxBusy=noiseEnabled && sfx.logicalBusy();']:
    assert q in main,q
fn=root/'data'/'sfx_capture_oracle_m225'/'sound_04.wav'
with wave.open(str(fn),'rb') as w: assert w.getnframes()*1000.0/w.getframerate()>20.0
assert '5,  // 4 movement' in sfx
for q in ['continuous short-horizon stream','do not clear or restart SDL', 'sound(4)']:
    assert q in notes.replace('\n',' '),q
print('M229 continuously-fed queued-audio / anti-bunching audit OK')
