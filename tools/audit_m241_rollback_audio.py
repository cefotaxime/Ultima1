#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
for q in ['pulsePlayedSampleLocked','pulseWriteActiveLocked','PA_SEEK_RELATIVE_ON_READ','PULSE_MAX_BURST_FRAMES']:
    assert q not in sfx,q
for q in ['transport=M238-direct-pulse-write-callback','PULSE_TARGET_LATENCY_US=5000','waitRenderedThrough(sourceEndSample,us+1000000u);']:
    assert q in sfx,q
for q in ['IMPULSE_QUALITY=150','IMPULSE_OS=32','SOUND4_INTERVALS_48K']:
    assert q in pc,q
assert 'SfxSystem();' in h
print('M241 rollback audio audit OK')
