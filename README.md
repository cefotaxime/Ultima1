# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M239**

이 프로젝트의 기준 실행 환경은 **WSL/Ubuntu에서 build_and_run_wsl.sh를 사용하는 경로**입니다.

M238 hotfix1에서 PC-speaker 음질은 정상으로 유지됐지만 첫 발소리는 여전히 약 3걸음 늦었습니다. M239은 합성음/피치/게임 속도를 건드리지 않고, direct PulseAudio 경로의 idle-silence buffering을 제거합니다.

- M236 hotfix1 PC-speaker synthesis SHA-256 고정
- idle 상태에서 Pulse write callback은 zero PCM을 쓰지 않음
- prebuf=0
- sound() 시작 시 최대 64 frames를 PA_SEEK_RELATIVE_ON_READ로 현재 read cursor에 즉시 prime
- 이후 callback은 PA_SEEK_RELATIVE로 append
- runtime WAV / SDL_QueueAudio / feeder thread / application PCM FIFO 없음
- audio_runtime_m239.txt에 실제 backend와 negotiated Pulse latency 정보 기록

M236 hotfix1 PcSpeakerEmulator.cpp SHA-256:
0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126
