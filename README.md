# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M229**

M229는 성/마을에서 여러 칸 이동한 뒤 발소리가 뒤늦게 뭉쳐 들리던 오디오 스케줄링 문제를 수정합니다.

- M225에서 검증된 DOSBox Staging 0.82.2 capture-oracle PCM과 sound ID mapping을 유지합니다.
- 사용자 WSLg에서 실제 출력이 확인된 `SDL_QueueAudio()` backend를 유지합니다.
- M228의 per-call `SDL_ClearQueuedAudio()` + queue 재제출 방식은 폐기했습니다.
- 별도 feeder thread가 1 ms 주기로 짧은 mixed PCM chunk만 공급합니다.
- capture/filter tail은 active voice로 유지되어 이후 효과음과 sample-wise overlap됩니다.
- sound(4)의 원본 source spacing 5 ms와 `logicalDurationMs()` 분리는 유지합니다.
- idle silence를 미리 길게 queue하지 않으므로 새 효과음이 기존 buffer 뒤로 밀리지 않습니다.
- 한 outer frame당 gameplay KEYDOWN 하나만 처리하는 M227 pacing을 유지합니다.
- M226의 zero-range Weapon Attack 메시지/BEEP 복원도 유지합니다.

실행 시 stderr에 SDL audio driver, obtained sample rate/buffer size, `queued-stream mixer=M229` 진단을 출력합니다.

세부 근거는 `ORACLE_NOTES_M229.md`와 `tools/audit_m229_streaming_queue_audio.py`를 참고하십시오.
