# Ultima I 한글화 clean-room — M244

M243 hotfix1은 현재 사용자 검증 기준 오디오 baseline이다. 음질, 한 이동당 효과음 수, 플레이어 행동 속도는 정상이며 WSL 환경에서 약 3걸음 정도의 체감 오디오 지연만 남아 있다.

M244는 **진단 전용 milestone**이다. 기본 실행에서는 M243의 오디오 재생 경로, PCM, PulseAudio buffer 설정, 동기 대기, 게임 루프를 변경하지 않는다.

`U1_AUDIO_PROBE=1`을 지정했을 때만 `audio_probe_m244.csv`에 효과음별 `play()` 시작/종료, 첫 Pulse render callback, Pulse latency/timing info, PCM onset을 기록한다. real-time callback에서는 파일 I/O를 하지 않는다.

M239/M240의 read-clock/reanchor/seek 방식과 M242의 playback-completion fence는 계속 금지 baseline으로 유지한다.
