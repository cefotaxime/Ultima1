# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M230**

M230는 M225~M229에서 사용하던 효과음 WAV 재생 중심 구조를 중단하고, 1987 DOS판 OUT.EXE의 PC-speaker sound(0)..sound(9) 루틴 자체를 source-level로 복원합니다.

- OUT.EXE file 0xB065..0xB155의 port 61h software-toggle 루프를 코드로 재구성했습니다.
- M225 direct capture WAV는 런타임 음원이 아니라 비교/검증 oracle로만 남깁니다.
- sound source duration은 DOSBox Staging 0.82.2 / cycles=3000 직접 녹음으로 보정합니다.
- finite speaker transition과 HP 120 Hz / LP 4300 Hz filter를 적용해 런타임 PCM을 생성합니다.
- SfxSystem::play()는 원본 sound()처럼 source-synchronous하게 동작합니다.
- local 이동은 원본 OUT.EXE 0x30E9..0x3120 순서대로 새 위치를 먼저 present한 뒤 sound(4)를 실행하고, 그 뒤 actor update로 넘어갑니다.
- host 전송은 짧은 continuous SDL queued stream을 유지하며 PULSE_LATENCY_MSEC=5를 요청합니다.
- M226 zero-range Weapon Attack 메시지/BEEP 복원도 유지합니다.

세부 근거는 ORACLE_NOTES_M230.md와 tools/audit_m230_pc_speaker_emulator.py를 참고하십시오.
