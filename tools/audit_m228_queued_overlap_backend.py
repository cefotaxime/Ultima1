#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
sfxh=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=228
assert 'want.callback' not in sfx
assert 'SDL_GetQueuedAudioSize(device_)' in sfx
assert 'std::vector<std::int16_t> queuedMix_' in sfxh
assert 'SDL_ClearQueuedAudio(device_)' in sfx
assert 'SDL_QueueAudio(device_,mixed.data(),bytes)' in sfx
assert 'SDL_QueueAudio(device_,clip.data' not in sfx
assert 'const std::uint32_t startDelayMs=remainingLogicalMs(now);' in sfx
assert 'logicalBusyUntilMs_=now+startDelayMs+durationMs;' in sfx
print('M228 queued-audio unplayed-tail remix audit OK')
