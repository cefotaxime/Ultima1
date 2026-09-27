#!/usr/bin/env python3
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc_path=root/'src/PcSpeakerEmulator.cpp'

assert hashlib.sha256(pc_path.read_bytes()).hexdigest() == '0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126'
for q in [
    'PULSE_TARGET_LATENCY_US=5000','PULSE_PRIME_FRAMES=64',
    'if(self->voices_.empty()) return;','attr.prebuf=0;',
    'void SfxSystem::pulseKick()','pa_stream_writable_size(state->stream)',
    'PA_SEEK_RELATIVE_ON_READ','PA_SEEK_RELATIVE)',
    'if(pulseAudio_) pulseKick();',
    'transport=M239-zero-idle-pulse-read-cursor',
    'audio_runtime_m239.txt'
]: assert q in sfx,q

assert sfx.count('PA_SEEK_RELATIVE_ON_READ') == 1
assert 'pulseKick();' in h
cb=sfx.index('pa_stream_set_write_callback(')
idle=sfx.index('if(self->voices_.empty()) return;',cb)
begin=sfx.index('pa_stream_begin_write(stream,&data,&writable)',cb)
assert cb < idle < begin
assert sfx.index('PA_SEEK_RELATIVE)',cb) < sfx.index('void SfxSystem::pulseKick()')
for q in ['SDL_QueueAudio(','SDL_GetQueuedAudioSize(','feederLoop(','pumpLocked(']:
    assert q not in sfx,q
print('M239 zero-idle PulseAudio read-cursor prime audit OK')
