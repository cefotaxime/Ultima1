# Ultima I 한글화 clean-room — M243

M242는 사용자 검증에서 명확한 회귀가 확인되어 폐기한다. 한 번의 이동 입력에 발소리가 두세 번 들리고, 플레이어 행동이 느려졌다. 원인은 M242에서 추가한 Pulse playback-completion fence가 게임 루프를 불필요하게 막은 데 있다.

M243은 마지막 검증 양호 상태인 **M241 hotfix1의 오디오 코어를 정확히 복원**한다. 새 latency 실험은 포함하지 않는다.

고정 사항:
- 기준 실행 환경: WSL/Ubuntu + `build_and_run_wsl.sh`
- `SfxSystem.cpp/.h`: M241 hotfix1과 동일
- `PcSpeakerEmulator.cpp/.h`: M241 hotfix1과 동일
- M242 `waitPulsePlayedThrough()` 제거
- M239/M240 read-clock/reanchor/seek 방식 없음
- 합성음, 피치, 필터, source duration 변경 없음

M243 hotfix1은 런타임 코드를 변경하지 않고, 구식 M226 audit가 요구하는 역사적 README milestone 표기 형식을 복원한다.
