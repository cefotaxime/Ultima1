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
for q in ['std::vector<int> delayWeights(int id)','w.assign(16,1000)','w.assign(48,355)',
          'for(int n=5;n<=255;++n)','w={5,10,15,20}','const bool immediate=(soundId==0 || soundId==1 || soundId==8)',
          'makeSecondOrderLowPass(4300.0,1.0)','makeSecondOrderHighPass(120.0,1.0)','TAIL_MS=32']:
    assert q in pc,q
assert 'SfxPcmOracleM225' not in sfx
u1=cmake.split('add_executable(u1kr_clean',1)[1].split(')',1)[0]
assert 'src/PcSpeakerEmulator.cpp' in u1 and 'src/SfxPcmOracleM225.cpp' not in u1
assert 'PULSE_LATENCY_MSEC' in sfx
queued=('SDL_QueueAudio(device_,chunk.data()' in sfx and 'playedSamplesLocked()<endSample' in sfx)
direct=('want.callback=&SfxSystem::audioCallback;' in sfx and 'app-queue=none' in sfx and 'waitSourceDurationUs(us);' in sfx)
assert queued or direct
assert 'bool localPlayerMoveSoundPending=false;' in main
assert 'if(noiseEnabled) localPlayerMoveSoundPending=true;' in main
assert 'if(localPlayerMoveSoundPending){' in main
print('M230 original PC-speaker routine emulator / draw-before-sound audit OK')
