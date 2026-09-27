# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M238**

이 프로젝트의 기준 실행 환경은 **WSL/Ubuntu에서 `build_and_run_wsl.sh`를 사용하는 경로**입니다. M237에서 추가한 native Windows IAudioClient3 경로는 보조 경로이며, 사용자의 실제 검증 환경을 대신하지 않습니다.

M238은 M236 hotfix1의 PC-speaker 합성음을 그대로 유지하면서 WSL/Linux 오디오 transport만 변경합니다.

- 48 kHz mono PC-speaker synthesis unchanged
- WSL/Linux primary path: direct asynchronous libpulse write callback
- `PA_STREAM_ADJUST_LATENCY`, target `tlength=5 ms`
- `minreq/prebuf/maxlength` are negotiated by PulseAudio
- `pa_stream_begin_write()` memory is rendered directly; no extra PCM FIFO
- SDL direct callback remains automatic fallback
- `U1_SFX_FORCE_SDL=1` forces the old SDL path for A/B testing
- runtime WAV playback / `SDL_QueueAudio` / feeder thread 없음
- `audio_runtime_m238.txt` records selected backend and negotiated Pulse buffer

M236 hotfix1의 `PcSpeakerEmulator.cpp` SHA-256은 계속 고정됩니다:

`0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126`

세부 내용은 `ORACLE_NOTES_M238.md`와 `tools/audit_m238_wsl_direct_pulse.py`를 참고하십시오.
