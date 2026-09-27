# M235 — source-synchronous SDL callback clock

M234 사용자 검증에서 DOSBox-style impulse 모델로 음질은 복원됐으나 local footstep audible onset은 약 7걸음 늦었다.

M234는 application PCM queue를 제거했지만 `play()`가 callback consumption과 무관하게 원본 source duration만 wall-clock으로 기다렸다. M235는 scheduling 시 callback sample clock의 `startSample`과 `sourceEndSample = startSample + clip.sourceSamples`를 기록하고, `renderedSamples_ >= sourceEndSample`이 될 때까지 `play()`가 반환하지 않는다.

M234의 direct SDL callback, app queue 없음, feeder 없음, WAV runtime 없음, DOSBox-style impulse renderer는 그대로 유지한다. sound(4)의 source span은 4.302 ms 그대로이며 audible filter tail은 source return 이후에도 다음 sound와 겹칠 수 있다.

M235는 SDL callback 이후 WSLg/PulseAudio/Windows presentation latency가 0이라고 가정하지 않는다.
