#!/usr/bin/env python3
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
sh=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
h=(root/'src/SfxSystem.h').read_text(encoding='utf-8')
pc_path=root/'src/PcSpeakerEmulator.cpp'
pc=pc_path.read_text(encoding='utf-8')

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=238
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=238

assert hashlib.sha256(pc_path.read_bytes()).hexdigest() == '0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126'
for q in ['IMPULSE_QUALITY=150','IMPULSE_OS=32','SOUND4_INTERVALS_48K','4302']:
    assert q in pc,q

for q in ['pkg_check_modules(PULSE REQUIRED libpulse)','U1_HAVE_LIBPULSE=1','${PULSE_LIBRARIES}']:
    assert q in cm,q
for q in ['libpulse-dev','pulseaudio-utils','audit_m238_wsl_direct_pulse.py','Starting clean-room M238 (canonical WSL runtime)']:
    assert q in sh,q

for q in [
    'pa_threaded_mainloop_new()','pa_context_connect(','pa_stream_new(',
    'pa_stream_set_write_callback(','pa_stream_begin_write(','pa_stream_connect_playback(',
    'PA_STREAM_ADJUST_LATENCY','PULSE_TARGET_LATENCY_US=5000',
    'attr.tlength=pa_usec_to_bytes(PULSE_TARGET_LATENCY_US,&ss)',
    'attr.minreq=static_cast<std::uint32_t>(-1)',
    'driver=pulseaudio-direct','U1_SFX_FORCE_SDL',
    'pa_stream_get_latency(','PulseAudio underflows=',
    'const auto logicalStartUs=perfUs();','const auto logicalEndUs=logicalStartUs+static_cast<std::uint64_t>(us)',
    'while(perfUs()<logicalEndUs) SDL_Delay(0);'
]:
    assert q in sfx,q
assert 'PULSE_TARGET_LATENCY_US=2000' not in sfx
for q in ['SDL_QueueAudio(','SDL_GetQueuedAudioSize(','feederLoop(','pumpLocked(']:
    assert q not in sfx,q

for q in ['want.callback=&SfxSystem::audioCallback;','transport=SDL-direct-callback-fallback','waitRenderedThrough(sourceEndSample,us+1000000u);']:
    assert q in sfx,q
for q in ['std::unique_ptr<PulseAudioState> pulseAudio_','initPulseLowLatency()','shutdownPulseLowLatency()']:
    assert q in h,q
assert 'SfxSystem();' in h
assert 'SfxSystem() = default;' not in h
assert 'SfxSystem::SfxSystem() = default;' in sfx

if len(sys.argv)>1:
    out=(Path(sys.argv[1])/'OUT.EXE').read_bytes(); assert len(out)==69858
    assert out[0x3119:0x3120]==bytes.fromhex('b8 04 00 50 e8 56 d5')
print('M238 canonical WSL direct-PulseAudio low-latency audit OK')
