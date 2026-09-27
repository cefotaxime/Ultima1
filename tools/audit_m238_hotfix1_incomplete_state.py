#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
s=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
assert 'SfxSystem();' in h
assert 'SfxSystem() = default;' not in h
assert 'struct PulseAudioState;' in h
assert 'std::unique_ptr<PulseAudioState> pulseAudio_;' in h
assert 'struct SfxSystem::PulseAudioState {' in s
assert s.index('struct SfxSystem::PulseAudioState {') < s.index('SfxSystem::SfxSystem() = default;')
print('M238 hotfix1 incomplete audio-state constructor audit OK')
