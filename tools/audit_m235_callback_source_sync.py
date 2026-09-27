#!/usr/bin/env python3
from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=235
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=235
for q in ['want.callback=&SfxSystem::audioCallback;','transport=SDL-direct-callback','app-queue=none',
          'sourceEndSample=startSample+static_cast<std::uint64_t>(clip.sourceSamples);',
          'waitRenderedThrough(sourceEndSample,us+1000000u);','renderedSamples_.load(std::memory_order_acquire)<endSample',
          'source_end=']:
    assert q in sfx,q
for q in ['std::atomic<std::uint64_t> renderedSamples_','std::atomic<std::uint64_t> synchronousEndSample_']:
    assert q in h,q
for q in ['SDL_QueueAudio(','SDL_GetQueuedAudioSize(','SDL_CreateThread(','feederLoop(','pumpLocked(']:
    assert q not in sfx,q
assert 'SOUND4_INTERVALS_48K' in pc and '4302' in pc
if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert out[0x3119:0x3120]==bytes.fromhex('b8 04 00 50 e8 56 d5')
print('M235 direct callback source-synchronous clock audit OK')
