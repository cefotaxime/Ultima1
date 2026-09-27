#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
sfxh=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=227
assert 'SDL_QueueAudio(device_,clip.data' not in sfx
for q in ['SDL_GetQueuedAudioSize(device_)','queuedMix_','mixed[dst]',
          'const std::uint32_t startDelayMs=remainingLogicalMs(now);']:
    assert q in sfx or q in sfxh,q
for q in ['bool keyDispatchedThisFrame=false;',
          'while(!sourceSfxBusy && !keyDispatchedThisFrame && SDL_PollEvent(&e)){',
          'const bool sourceSfxBusy=noiseEnabled && sfx.logicalBusy();']:
    assert q in main,q
print('M227 overlap/input-pacing behavior audit OK (M228 queued-remix backend)')
