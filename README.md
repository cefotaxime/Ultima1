# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M233**

M233는 M232 사용자 검증에서 발소리 지연이 약 4걸음에서 7.5걸음으로 악화된 결과를 반영해, M232의 공격적인 low-latency transport 실험을 폐기하고 M231 transport로 복귀합니다.

- idle silence prefill 제거는 유지합니다.
- SDL requested period는 64 frames로 복원합니다.
- feeder cadence는 1 ms로 복원합니다.
- `PULSE_LATENCY_MSEC`는 5로 복원합니다.
- M232에서 수정한 sound(4) 피치 보정은 유지합니다.
- sound(4)의 synchronous source span 4.302 ms는 유지합니다.
- `U1_SFX_TRACE=1` 계측도 유지합니다.

즉 M233은 **M231 transport + M232 footstep pitch correction** 조합입니다. 남은 WSLg downstream 지연이 해결되었다고 가정하지 않습니다.

세부 근거는 `ORACLE_NOTES_M233.md`와 `tools/audit_m233_m231_transport_m232_pitch.py`를 참고하십시오.
