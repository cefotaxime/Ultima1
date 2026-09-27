# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M228**

M228은 M227에서 발생한 효과음 완전 무음 회귀를 수정합니다.

- M225에서 실제 WSLg 출력이 확인된 `SDL_QueueAudio()` backend로 복귀합니다.
- M225처럼 complete capture WAV를 FIFO로 직렬 연결하지는 않습니다.
- `SDL_GetQueuedAudioSize()`로 아직 재생되지 않은 queue suffix를 보존하고, 새 PCM을 원본 `logicalDurationMs()` offset에 sample-wise mix한 뒤 queue를 교체합니다.
- 따라서 sound(4)의 긴 impulse/filter tail은 다음 sound(4)와 겹치며 전체 WAV 길이가 다음 효과음 시작을 막지 않습니다.
- M227의 outer-frame당 최대 하나의 KEYDOWN 정책과 source-level logical busy gate는 유지합니다.
- M225 capture-oracle PCM, sound ID mapping, M226 zero-range Weapon Attack 복원은 유지합니다.
- host audio device 초기화 실패는 stderr에 표시하도록 배포 소스에서 진단성을 추가했습니다.

세부 근거는 `ORACLE_NOTES_M228.md`와 `tools/audit_m228_queued_overlap_backend.py`를 참고하십시오.

## WSL 실행

배포 ZIP의 `README_KO.md`와 `build_and_run_wsl.sh`를 사용하십시오.
