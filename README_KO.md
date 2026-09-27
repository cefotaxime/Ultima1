# Ultima I 한글화 clean-room — M246

M245에서 WSLg/RDPSink를 우회하면 delay가 완전히 사라진다는 것은 사용자 검증으로 확인됐다. 다만 Windows의 구식 `waveOut` 출력 경로로 바꾸면서 승인된 PC-speaker 효과음의 음색이 달라졌다.

M246은 WSL→Windows host 우회 구조는 유지하되 `waveOut`을 제거하고 event-driven WASAPI를 사용한다. Windows companion은 먼저 기본 출력 장치를 48 kHz / stereo / 16-bit PCM exclusive mode로 열고, 장치가 보고한 minimum period를 그대로 사용한다. 게임에서 만든 승인된 mono PCM sample 값은 좌/우 채널에 동일하게 복제할 뿐이며 resampling, filter, gain scaling, PCM 재생성은 하지 않는다.

exclusive WASAPI가 허용되지 않으면 event-driven shared WASAPI를 시도하고, 그것도 실패하면 M243 direct PulseAudio 경로로 자동 fallback한다. `audio_runtime_m246.txt`에 실제 모드가 기록된다.
