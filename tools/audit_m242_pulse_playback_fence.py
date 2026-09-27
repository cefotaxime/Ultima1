#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc=(root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8')
for q in [
    'void waitPulsePlayedThrough(std::uint64_t endSample,std::uint32_t watchdogUs);',
    'void SfxSystem::waitPulsePlayedThrough(std::uint64_t endSample,std::uint32_t watchdogUs)',
    'pa_stream_get_latency(pulseAudio_->stream,&latencyUs,&negative)',
    'const auto produced=renderedSamples_.load(std::memory_order_acquire);',
    'if(played>=endSample) return;',
    'waitRenderedThrough(sourceEndSample,us+1000000u);',
    'if(pulseAudio_) waitPulsePlayedThrough(sourceEndSample,1000000u);',
]:
    assert q in (h+'\n'+sfx),q
for q in ['PA_SEEK_RELATIVE_ON_READ','pulsePlayedSampleLocked','pulseWriteActiveLocked','PULSE_MAX_BURST_FRAMES']:
    assert q not in sfx,q
for q in ['PULSE_TARGET_LATENCY_US=5000','transport=M238-direct-pulse-write-callback',
          'pa_stream_set_write_callback(','pa_stream_begin_write(','PA_STREAM_ADJUST_LATENCY']:
    assert q in sfx,q
for q in ['IMPULSE_QUALITY=150','IMPULSE_OS=32','SOUND4_INTERVALS_48K']:
    assert q in pc,q
print('M242 Pulse playback-fence / unchanged-PCM audit OK')
