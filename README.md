# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M234**

M234는 WAV 런타임 재생으로 돌아가지 않고, 원본 OUT.EXE PC-speaker 루틴을 source oracle로 유지한 채 음색 생성과 SDL 전달 경로를 다시 구성합니다.

- 런타임 음원은 `PcSpeakerEmulator`가 생성합니다. M225 capture WAV는 비교/검증 oracle 전용입니다.
- M233의 임의 70 us speaker slew + 2차 HP/LP 모델을 폐기했습니다.
- DOSBox Staging 0.82.2 impulse speaker 구조를 기준으로 raised-cosine-windowed sinc 적분 모델을 사용합니다.
- PC-speaker filter는 120 Hz 3차 high-pass + 4300 Hz 3차 low-pass로 구성합니다.
- sound(4)는 M225 capture에서 확인한 21~22 sample toggle cadence를 유지하고, source blocking span 4.302 ms는 변경하지 않습니다.
- `SDL_QueueAudio`, feeder thread, application PCM queue를 제거했습니다.
- SDL device가 요청할 때 `audioCallback`에서 직접 현재 voice를 render하므로 application-side idle prefill은 0 samples입니다.
- `U1_SFX_TRACE=1` 진단은 유지합니다.

M234는 WSLg/PulseAudio/Windows 내부 buffering까지 제거했다고 가정하지 않습니다. 세부 근거는 `ORACLE_NOTES_M234.md`와 `tools/audit_m234_direct_callback_impulse.py`를 참고하십시오.
