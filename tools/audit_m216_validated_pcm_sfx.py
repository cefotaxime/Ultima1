#!/usr/bin/env python3
from pathlib import Path
import re,sys,wave
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
sfxh=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pcmh=(root/'src/SfxPcmOracleM225.h').read_text(encoding='utf-8')
pcmc=(root/'src/SfxPcmOracleM225.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
build=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=216
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cmake); assert pm and int(pm.group(1))>=216

# M216's architectural rule remains: runtime playback is fixed PCM, not a
# second host-dependent speaker synthesizer. M225 supersedes the old M212
# synthetic buffers with direct DOSBox capture-oracle PCM. M229 keeps that
# rule while feeding short mixed chunks through the proven queued-audio device.
for q in ['#include "SfxPcmOracleM225.h"','SfxPcmOracleM225::clip(soundId)',
          'want.freq=SfxPcmOracleM225::SAMPLE_RATE','want.samples=64',
          'want.callback=nullptr','SDL_GetQueuedAudioSize(device_)',
          'SDL_CreateThread(&SfxSystem::feederEntry',
          'voices_.push_back(Voice{clip.data,clip.samples,startSample})']:
    assert q in sfx,q
for q in ['DOSBOX074_EFFECTIVE_8088_HZ','SPKR_SLEW_SECONDS','renderDosboxSpeaker','renderOriginalSound']:
    assert q not in sfx,q
assert 'constexpr int SAMPLE_RATE = 48000;' in pcmh
assert 'src/SfxPcmOracleM225.cpp' in cmake
assert 'logicalDurationMs' in sfxh
assert 'SDL_QueueAudio(device_,chunk.data()' in sfx
assert 'SDL_QueueAudio(device_,clip.data' not in sfx

# M229: source calls enqueue voices only; the feeder performs short-horizon
# queue writes. Per-call queue replacement from M228 must not return.
sched=sfx[sfx.index('void SfxSystem::schedule'):sfx.index('bool SfxSystem::logicalBusy')]
assert 'SDL_QueueAudio' not in sched
assert 'SDL_ClearQueuedAudio' not in sched

assert 'audit_m216_validated_pcm_sfx.py' in build
for i in range(10): assert f'case {i}: return {{SOUND_{i:02d}' in pcmc

assert 'sfx.logicalDurationMs(worldProjectileFirstSoundId)' in main
assert 'std::max(stepHoldMs,sfx.logicalDurationMs(worldProjectileFirstSoundId))' not in main

ref=root/'data/sfx_capture_oracle_m225'
for i in range(10):
    with wave.open(str(ref/f'sound_{i:02d}.wav'),'rb') as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
        assert w.getnframes()>0

if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert bytes.fromhex('b8 07 00 50 ff 76 06 e8 44 ff 8b e5') == out[0x1324:0x1330]
    assert bytes.fromhex('b8 02 00 50 e8 67 f5 8b e5') == out[0x1108:0x1111]
print('M216 fixed-PCM architecture compatibility audit OK (M229 stream feeder)')
