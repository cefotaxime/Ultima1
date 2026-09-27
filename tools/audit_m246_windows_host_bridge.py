#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
ps=(root/'tools/u1_audio_host.ps1').read_text(encoding='utf-8')
for q in [
    'driver=windows-host-wasapi-bridge',
    'transport=M246-WSL-interop-host-wasapi',
    'pcm=M241-approved',
    'audio_runtime_m246.txt',
    'state->backend=line.substr(6)',
    'U1_SFX_FORCE_PULSE',
    'initHostAudioBridge()',
    'sendHostAudioSound(int soundId)',
]:
    assert q in (sfx+'\n'+h),q
for q in [
    'IAudioClient','IAudioRenderClient','IMMDeviceEnumerator',
    'AUDCLNT_STREAMFLAGS_EVENTCALLBACK',
    'AUDCLNT_SHAREMODE_EXCLUSIVE','AUDCLNT_SHAREMODE_SHARED',
    'GetDevicePeriod','MinimumPeriodHns',
    'Client.Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE',
    'Client.Initialize(AUDCLNT_SHAREMODE_SHARED',
    'Client.SetEventHandle','Client.GetCurrentPadding',
    'Render.GetBuffer','Render.ReleaseBuffer',
    'stereo[i * 2] = v','stereo[i * 2 + 1] = v',
    'AvSetMmThreadCharacteristics("Pro Audio"',
    'CoInitializeEx(IntPtr.Zero, COINIT_MULTITHREADED)',
]:
    assert q in ps,q
for q in ['waveOutOpen','waveOutWrite','waveOutPrepareHeader','timeBeginPeriod']:
    assert q not in ps,q
for q in ['PA_SEEK_RELATIVE_ON_READ','waitPulsePlayedThrough','pulsePlayedSampleLocked','pulseWriteActiveLocked']:
    assert q not in sfx,q
assert 'transport=M238-direct-pulse-write-callback' in sfx
print('M246 Windows-host event-driven WASAPI bridge audit OK')
