# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M227**

이 저장소는 1987 DOS판 *Ultima I*의 실행파일/데이터 동작을 oracle로 삼아 SDL2로 재구성하고 한글화하는 프로젝트입니다. 원본 게임 데이터와 폰트 바이너리는 저장소에 포함하지 않습니다.

## M227

M225의 DOSBox Staging 0.82.2 capture-oracle PCM 자체는 유지하고, 재생 스케줄링을 원본의 동기식 PC-speaker 호출 구조에 맞게 수정했습니다.

- 전체 capture WAV를 `SDL_QueueAudio()` FIFO로 직렬화하던 구조를 callback mixer로 교체했습니다.
- 원본 `sound()` source call은 `logicalDurationMs()` 간격으로 이어지고, 더 긴 impulse/filter tail은 서로 겹쳐 재생됩니다.
- `playImmediate()`는 이전 audible tail을 잘라내지 않으며, 앞선 source call이 끝난 뒤 가능한 즉시 시작합니다.
- 성/마을의 player/NPC sound(4)는 약 5 ms source 간격으로 시작하며 39 ms 안팎의 capture tail은 overlap됩니다.
- 한 outer frame에서 최대 하나의 KEYDOWN만 처리하여 여러 이동 command가 한 번의 `SDL_RenderPresent()` 전에 몰아서 실행되지 않게 했습니다.
- source-level SFX가 논리적으로 진행 중일 때 gameplay key는 SDL queue에 남겨 둡니다. audible tail은 입력 hold에 포함하지 않습니다.
- M226의 zero-range Weapon Attack 메시지/BEEP 복원은 그대로 유지합니다.

세부 근거는 `ORACLE_NOTES_M227.md`와 `tools/audit_m227_sfx_overlap_input_pacing.py`를 참고하십시오.

## WSL 실행

배포 ZIP의 `README_KO.md`와 `build_and_run_wsl.sh`를 사용하십시오.
