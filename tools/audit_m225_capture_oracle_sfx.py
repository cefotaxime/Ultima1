#!/usr/bin/env python3
from pathlib import Path
import re, wave, sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
ora=(root/'src/SfxOracle.h').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
sfxh=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pcmh=(root/'src/SfxPcmOracleM225.h').read_text(encoding='utf-8')
pcmc=(root/'src/SfxPcmOracleM225.cpp').read_text(encoding='utf-8')
notes=(root/'ORACLE_NOTES_M225.md').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
readme=(root/'README_KO.md').read_text(encoding='utf-8')

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=225
m=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cmake); assert m and int(m.group(1))>=225
assert 'src/SfxPcmOracleM225.cpp' in cmake
assert 'constexpr int SAMPLE_RATE = 48000;' in pcmh
assert '#include "SfxPcmOracleM225.h"' in sfx
assert 'SfxPcmOracleM225::clip(soundId)' in sfx
assert 'want.callback=&SfxSystem::audioCallback' in sfx
assert 'SDL_QueueAudio' not in sfx and 'SDL_ClearQueuedAudio' not in sfx

ref=root/'data'/'sfx_capture_oracle_m225'
for sid in range(10):
    fn=ref/f'sound_{sid:02d}.wav'
    assert fn.is_file(),fn
    with wave.open(str(fn),'rb') as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,48000)
        assert w.getnframes()>0
    assert f'case {sid}: return {{SOUND_{sid:02d}' in pcmc

for q in [
    'InvalidAttackWeapon      = 1',
    'CombatHit                = 2',
    'TrapTriggered             = 2',
    'PositiveEffect           = 5',
    'NoEffect                 = 6',
    'PlayerAttackLaunch       = 7',
    'VehicleFire              = 8',
    'SpaceFlightPulse         = 9',
]: assert q in ora,q

assert 'logicalDurationMs(int soundId)' in sfxh
for q in ['48,','51,','78,','99,','5,','900,','40,','249,','98,','1,']:
    assert q in sfx,q
assert 'sfx.logicalDurationMs(worldProjectileFirstSoundId)' in main
assert 'sfx.logicalDurationMs(SfxEvent::EnemyRangedLaunch)' in main
assert 'sfx.logicalDurationMs(SfxEvent::CombatHit)' in main
assert 'durationMs(' not in sfxh
assert 'std::vector<Voice> voices_' in sfxh
assert 'mixed+=v.data' in sfx

assert 'pendingAttackResolveMode==2 ? sfxId(SfxEvent::VehicleFire) : -1' in main
assert 'VehicleFire              = 8' in ora
assert 'VehicleFire              = 6' not in ora

for q in [
    'beginCommandSfxHold(SfxEvent::PositiveEffect',
    'beginCommandSfxHold(SfxEvent::NoEffect',
    'beginCommandSfxHold(SfxEvent::TrapTriggered',
]: assert q in main,q

for q in ['PCM 길이와 게임 blocking 시간 분리','VehicleFire = 8','직접 녹음한 capture PCM']:
    assert q in notes,q
m=re.search(r'Ultima I 한글화 clean-room — M(\d+)',readme); assert m and int(m.group(1))>=225

if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert out[0x5657:0x565B] == bytes.fromhex('b8 08 00 50')
    assert bytes.fromhex('b8 02 00 50 e8 67 f5 8b e5') == out[0x1108:0x1111]

print('M225 DOSBox capture-oracle SFX / logical-duration split audit OK')
