# M234 — direct SDL callback + DOSBox-style impulse speaker

M233 사용자 검증: footstep 지연은 약 4칸으로 회복됐지만, 재구성된 PC-speaker 음색은 M225 direct capture보다 피치가 낮고 잡음이 많았다.

M234는 두 문제를 분리한다.

1. **음색**: M233의 70 us finite-slew + 2차 biquad 조합을 제거했다. DOSBox Staging 0.82.2의 impulse speaker가 사용하는 raised-cosine-windowed sinc 적분 구조를 기준으로 48 kHz 직접 출력 모델을 만들고, 3차 120 Hz HP / 4300 Hz LP 필터를 적용한다. 런타임 WAV playback은 사용하지 않는다.
2. **지연**: `SDL_QueueAudio`, feeder thread, `SDL_GetQueuedAudioSize` 기반 clock을 모두 제거했다. SDL device가 요청할 때 callback이 즉시 현재 voice를 render한다. application-side queued PCM과 idle prefill은 0이다.

M225 capture는 파형의 pitch/level/tone을 비교하는 oracle로만 남는다. sound(4)의 source blocking span 4.302 ms는 변경하지 않는다.

남는 지연은 SDL device/backend/WSLg/PulseAudio/Windows audio engine 쪽에서 생길 수 있으며 M234는 이를 해결됐다고 가정하지 않는다.
