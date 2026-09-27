#!/usr/bin/env python3
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc_path=root/'src/PcSpeakerEmulator.cpp'
pc=pc_path.read_text(encoding='utf-8')

assert hashlib.sha256(pc_path.read_bytes()).hexdigest() == '0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126'
for q in ['IMPULSE_QUALITY=150','IMPULSE_OS=32','SOUND4_INTERVALS_48K','4302']:
    assert q in pc,q
for q in [
    'IAudioClient3','GetSharedModeEnginePeriod','minPeriodFrames',
    'InitializeSharedAudioStream(', 'AUDCLNT_STREAMFLAGS_EVENTCALLBACK',
    'SetEventHandle(', 'IAudioRenderClient', 'GetCurrentPadding(',
    'AvSetMmThreadCharacteristicsW(L"Pro Audio"',
    'transport=M237-event-driven-low-latency', 'app-queue=none'
]:
    assert q in sfx,q
assert 'state->minPeriodFrames,mix,nullptr' in sfx
assert 'mix->nSamplesPerSec!=PcSpeakerEmulator::SAMPLE_RATE' in sfx
assert 'preserving pitch via SDL fallback' in sfx
for q in [
    'want.callback=&SfxSystem::audioCallback;',
    'want.userdata=this;',
    'transport=SDL-direct-callback-fallback',
    'waitRenderedThrough(sourceEndSample,us+1000000u);'
]:
    assert q in sfx,q
for q in ['SDL_QueueAudio(','SDL_GetQueuedAudioSize(','SDL_CreateThread(','feederLoop(','pumpLocked(']:
    assert q not in sfx,q
for q in ['std::mutex voicesMutex_','std::atomic<bool> enabled_','std::unique_ptr<WinAudioState> winAudio_']:
    assert q in h,q
assert 'std::lock_guard<std::mutex> lock(voicesMutex_)' in sfx
print('M237 Windows IAudioClient3 minimum-period low-latency audit OK')
