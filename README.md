# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M235**

M235는 M234에서 복원된 DOSBox-style impulse PC-speaker 음질과 direct SDL callback 경로를 유지하면서, 원본 `sound()`의 동기성을 callback sample clock에 다시 결합합니다.

- WAV는 런타임 음원으로 사용하지 않습니다. M225 capture는 비교/검증 oracle 전용입니다.
- `SDL_QueueAudio`, feeder thread, application PCM queue, idle silence prefill은 없습니다.
- 각 sound 호출은 callback의 현재 `renderedSamples_`를 start로 잡고 `sourceEnd = start + clip.sourceSamples`를 계산합니다.
- `play()`는 SDL callback이 실제로 `sourceEnd`를 render할 때까지 반환하지 않습니다.
- sound(4)의 source span 4.302 ms와 M234의 DOSBox-style impulse 음질 모델은 유지합니다.
- filter tail은 source return 이후 다음 sound와 겹칠 수 있습니다.
- `U1_SFX_TRACE=1`에서 rendered/source_end/callback_frames를 확인할 수 있습니다.

M235는 SDL callback 이후 WSLg/PulseAudio/Windows의 물리적 presentation latency까지 제거됐다고 가정하지 않습니다.
