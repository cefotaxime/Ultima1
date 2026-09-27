#!/usr/bin/env python3
from pathlib import Path
import hashlib,re
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
ps=(root/'tools/u1_audio_host.ps1').read_text(encoding='utf-8')
sh=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
assert hashlib.sha256((root/'src/PcSpeakerEmulator.cpp').read_bytes()).hexdigest() == '0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126'
assert hashlib.sha256((root/'src/PcSpeakerEmulator.h').read_bytes()).hexdigest() == 'b7f5b4f0663671975b98bd16abb30f7595fd50c866a57a77eca2c7e949beeb01'
assert hashlib.sha256((root/'src/SfxSystem.cpp').read_bytes()).hexdigest() == '551bfd242012cd80e3a30981a8c9b33941f696d9dec5f35fbdac76ef5463ca2c'
assert hashlib.sha256((root/'src/SfxSystem.h').read_bytes()).hexdigest() == 'c50ae46a11a136019960c95bcddfc15dfbb70a04dababf6402537048d3f40a68'
for q in ['HostAudioBridgeState','initHostAudioBridge()','sendHostAudioSound(int soundId)','U1_SFX_FORCE_PULSE',
          'driver=windows-host-waveout-bridge','transport=M245-WSL-interop-host-audio','pcm=M241-approved',
          'powershell.exe -NoProfile -ExecutionPolicy Bypass','u1_audio_host.ps1',
          "const char magic[4]={'U','1','A','1'}",'speaker_.clip(id)','waitFallbackDurationUs(us);']:
    assert q in (sfx+'\n'+h),q
for q in ['waveOutOpen','waveOutPrepareHeader','waveOutWrite','waveOutReset','waveOutClose',
          'BUFFER_FRAMES = 128','BUFFER_COUNT = 3','SAMPLE_RATE = 48000','LoadClips','Voices.Add(new Voice(Clips[id], ProducedFrame))',
          'AvSetMmThreadCharacteristics("Pro Audio"','timeBeginPeriod(1)']:
    assert q in ps,q
for forbidden in ['SOUND4_INTERVALS_48K','IMPULSE_QUALITY','PA_SEEK_RELATIVE_ON_READ']:
    assert forbidden not in ps,forbidden
for forbidden in ['waitPulsePlayedThrough','PA_SEEK_RELATIVE_ON_READ','pulsePlayedSampleLocked','pulseWriteActiveLocked','PULSE_MAX_BURST_FRAMES']:
    assert forbidden not in sfx,forbidden
for q in ['transport=M238-direct-pulse-write-callback','PULSE_TARGET_LATENCY_US=5000','initPulseLowLatency()']:
    assert q in (sfx+'\n'+h),q
print('M245 Windows-host audio bridge / exact-PCM audit OK')
