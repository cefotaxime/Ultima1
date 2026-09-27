# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M236**

M236은 M235의 오디오 합성/동기화 코드를 바꾸지 않고, 남은 약 6걸음 audible latency가 WSLg/PulseAudio 경로인지 비교하기 위한 native Windows SDL2/WASAPI 실행 경로를 추가합니다.

- runtime WAV playback 없음; M225 capture는 oracle 전용
- M234 DOSBox-style impulse PC-speaker 음질 유지
- M235 direct SDL callback + callback sample-clock synchronization 유지
- SDL_QueueAudio / feeder thread / application PCM queue / idle silence prefill 없음
- Windows 비교 경로: SDL2/WASAPI → Windows audio engine → device
- WSL 비교 경로: SDL2 → PulseAudio/WSLg → Windows audio engine → device
- `build_and_run_windows.ps1` + `vcpkg.json` 추가
- Windows 실행은 `SDL_AUDIODRIVER=wasapi`를 강제하며 `-Trace`로 기존 SFX trace를 활성화할 수 있음

세부 내용은 `ORACLE_NOTES_M236.md`와 `tools/audit_m236_windows_wasapi.py`를 참고하십시오.
