#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
for forbidden in ['waitPulsePlayedThrough','PA_SEEK_RELATIVE_ON_READ','pulsePlayedSampleLocked','pulseWriteActiveLocked','PULSE_MAX_BURST_FRAMES']:
    assert forbidden not in sfx,forbidden
for required in ['transport=M238-direct-pulse-write-callback','PULSE_TARGET_LATENCY_US=5000','waitRenderedThrough(sourceEndSample,us+1000000u);']:
    assert required in sfx,required
for required in ['IMPULSE_QUALITY=150','IMPULSE_OS=32','SOUND4_INTERVALS_48K']:
    assert required in pc,required
assert 'SfxSystem();' in h
print('M243 exact M241-hotfix1 audio baseline audit OK')
