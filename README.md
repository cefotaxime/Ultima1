# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M232**

M232는 M231에서 확인된 idle-silence prefill 제거를 유지하면서, 애플리케이션 내부에서 아직 줄일 수 있던 오디오 lead와 sound(4) 피치 회귀를 분리해서 수정합니다.

- active/future voice가 없을 때는 PCM을 한 sample도 미리 queue하지 않습니다.
- SDL requested period를 64 → 32 frames로 줄였습니다(48 kHz 기준 약 1.33 → 0.67 ms).
- active feeder cadence를 1 ms → 250 us로 줄였습니다.
- PulseAudio latency request를 5 → 2 ms로 낮췄습니다.
- M230/M231의 sound(4)는 4.302 ms source span 전체로 toggle train을 stretch해 피치가 낮아졌습니다.
- M232는 M225 DOSBox direct capture의 onset/주파수에 맞춰 sound(4) toggle train을 보정하면서 source blocking span 4.302 ms는 그대로 유지합니다.
- offline probe: capture onset 12 samples / generated onset 12 samples, dominant FFT bin 1125 Hz / 1125 Hz.
- U1_SFX_TRACE=1 계측은 계속 유지합니다.

세부 근거는 `ORACLE_NOTES_M232.md`와 `tools/audit_m232_audio_latency_pitch.py`를 참고하십시오.
