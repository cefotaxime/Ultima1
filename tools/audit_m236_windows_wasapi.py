#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
ps=(root/'build_and_run_windows.ps1').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
vcp=(root/'vcpkg.json').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=236
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=236
assert 'if(WIN32)' in cm
assert 'find_package(SDL2 CONFIG REQUIRED)' in cm
assert 'TARGET SDL2::SDL2' in cm
assert 'pkg_check_modules(SDL2 REQUIRED sdl2)' in cm
for q in ['SDL_AUDIODRIVER', "'wasapi'", 'CMAKE_TOOLCHAIN_FILE', 'VCPKG_TARGET_TRIPLET=x64-windows', 'u1kr_clean.exe', 'U1_SFX_TRACE']:
    assert q in ps,q
assert '"sdl2"' in vcp
# M235 SDL/WASAPI comparison/fallback architecture must remain available.
for q in ['want.callback=&SfxSystem::audioCallback;','app-queue=none','waitRenderedThrough(sourceEndSample,us+1000000u);']:
    assert q in sfx,q
for q in ['SDL_QueueAudio(','SDL_GetQueuedAudioSize(','SDL_CreateThread(']:
    assert q not in sfx,q
assert 'SOUND4_INTERVALS_48K' in pc
print('M236 native Windows SDL2/WASAPI comparison/fallback-path audit OK')
