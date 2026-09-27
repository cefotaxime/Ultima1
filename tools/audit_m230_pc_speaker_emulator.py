#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
if len(sys.argv)!=2: raise SystemExit('usage: audit_m230_pc_speaker_emulator.py GAMEDATA')
out=(Path(sys.argv[1])/'OUT.EXE').read_bytes()
assert out[0x676:0x68A]==bytes.fromhex('55 8b ec 83 3e b0 3a 00 74 08 ff 76 04 e8 c2 a9 8b e5 5d c3')
assert out[0xFB4A:0xFB5E]==bytes.fromhex('65 ae 7d ae 9b ae a2 ae a9 ae c9 ae d0 ae ea ae 1e af 37 af')
for q in ['w.assign(16,1000)','w.assign(48,355)','for(int n=5;n<=255;++n)','w={5,10,15,20}',
          'makeLowPass(4300.0','makeHighPass(120.0','FILTER_TAIL_MS=32']:
    assert q in pc,q
assert 'SfxPcmOracleM225' not in sfx
assert 'PULSE_LATENCY_MSEC' in sfx
assert 'while(feederRun_ && enabled_ && playedSamplesLocked()<endSample)' in sfx
assert 'sourceCursorSample_=start+clip.sourceSamples;' in sfx
assert 'bool localPlayerMoveSoundPending=false;' in main
assert 'if(noiseEnabled) localPlayerMoveSoundPending=true;' in main
assert 'if(localPlayerMoveSoundPending){' in main
assert 'src/PcSpeakerEmulator.cpp' in cmake
print('M230 original PC-speaker routine emulator audit OK')
