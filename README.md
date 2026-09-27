# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M231**

M231은 M230 사용자 검증에서 계속 재현된 local footstep 지연을 “해결됐다고 가정”하지 않고 원인별로 계측하기 위한 단계입니다.

- M225 oracle sound_04.wav의 첫 유의미 sample은 약 0.25 ms입니다.
- M230+ PC-speaker emulator가 생성하는 sound(4)의 첫 nonzero sample은 약 0.48 ms입니다.
- 따라서 수십~수백 ms짜리 leading silence는 source PCM 자체에 없습니다.
- 대신 M230 feeder가 active sound가 없을 때도 한 SDL device period의 0 PCM을 미리 queue하는 결함을 확인했습니다.
- M231은 idle silence prefill을 제거합니다.
- U1_SFX_TRACE=1에서 play_begin / schedule / play_end / queued bytes / sample position을 microsecond timestamp로 기록합니다.
- build_and_run_wsl.sh는 pactl이 있으면 PulseAudio/WSLg sink의 reported/configured latency를 출력합니다.
- source-synchronous PC-speaker emulator와 M226 공격 BEEP 복원은 유지합니다.

세부 근거는 ORACLE_NOTES_M231.md와 tools/audit_m231_audio_lead_probe.py를 참고하십시오.
