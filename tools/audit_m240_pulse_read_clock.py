#!/usr/bin/env python3
from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc=root/'src/PcSpeakerEmulator.cpp'
assert hashlib.sha256(pc.read_bytes()).hexdigest() == '0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126'
for q in [
    'PULSE_MAX_BURST_FRAMES=240',
    'activeEnd<=base) return 0',
    'pulsePlayedSampleLocked()',
    'pa_stream_get_latency(pulseAudio_->stream,&latencyUs,&negative)',
    'if(negative) return produced+frames;',
    'renderedSamples_.store(startSample,std::memory_order_release);',
    'pulseWriteActiveLocked(true,static_cast<std::size_t>(-1));',
    'self->pulseWriteActiveLocked(false,nbytes);',
    'const auto mode=reanchor ? PA_SEEK_RELATIVE_ON_READ : PA_SEEK_RELATIVE;',
    'attr.prebuf=0;',
    'PA_STREAM_START_CORKED',
    'pa_stream_cork(state->stream,0,',
    'transport=M240-pulse-read-clock-no-idle-prefill',
    'audio_runtime_m240.txt'
]:
    assert q in sfx,q
assert 'pulseKick()' not in sfx
for q in ['pulseWriteActiveLocked(bool reanchor,std::size_t writableHintBytes)',
          'pulsePlayedSampleLocked() const']:
    assert q in h,q
print('M240 PulseAudio playback-read-clock reanchor audit OK')
