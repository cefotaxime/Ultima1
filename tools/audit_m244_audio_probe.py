#!/usr/bin/env python3
from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
s=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
assert hashlib.sha256((root/'src/SfxSystem.cpp').read_bytes()).hexdigest() == '8669e0c8b1539d2f94db4f7ecd0b454e03fd6fb45555b7dcd705509993c6bce0'
assert hashlib.sha256((root/'src/SfxSystem.h').read_bytes()).hexdigest() == '24da09c06d7ef72a97afbd44eb2250283cfb72222731f5b278abe06376bcade6'
assert hashlib.sha256((root/'src/PcSpeakerEmulator.cpp').read_bytes()).hexdigest() == '0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126'
for q in ['U1_AUDIO_PROBE','audio_probe_m244.csv','probePlayArmed_','probeFirstWriteUs_','probeFirstWriteFrames_',
          'pa_stream_get_latency(pulseAudio_->stream,&latencyUs,&negative)',
          'pa_stream_get_timing_info(pulseAudio_->stream)','first_nonzero_pcm_us','first_abs500_pcm_us']:
    assert q in (s+'\n'+h),q
for q in ['PA_SEEK_RELATIVE_ON_READ','pulsePlayedSampleLocked','pulseWriteActiveLocked','waitPulsePlayedThrough','PULSE_MAX_BURST_FRAMES']:
    assert q not in s,q
for q in ['PULSE_TARGET_LATENCY_US=5000','pa_stream_write(stream,data,writable,nullptr,0,PA_SEEK_RELATIVE)',
          'waitRenderedThrough(sourceEndSample,us+1000000u);','while(perfUs()<logicalEndUs) SDL_Delay(0);',
          'transport=M238-direct-pulse-write-callback']:
    assert q in s,q
cb=s[s.index('void SfxSystem::renderCallback'):s.index('void SfxSystem::waitFallbackDurationUs')]
assert 'ofstream' not in cb and 'audioDiagAppend' not in cb and 'audio_probe_m244.csv' not in cb
print('M244 non-invasive audio latency probe audit OK')
