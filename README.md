# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M237**

M236 hotfix1의 native Windows 실행에서 PC-speaker 음질은 유지되면서 첫 발소리 audible latency가 약 6걸음에서 3걸음으로 감소했다. M237은 합성음을 더 조정하지 않고 Windows 출력 transport만 줄인다.

- M236 hotfix1 PcSpeakerEmulator.cpp는 byte-for-byte 유지
- Windows 기본 경로: IAudioClient3 shared mode + minimum engine period + event-driven IAudioRenderClient
- MMCSS "Pro Audio" scheduling 사용(가능한 경우)
- 48 kHz mix format만 low-latency 경로에서 사용하여 피치 보존
- 초기화 실패/비 48 kHz endpoint는 기존 SDL/WASAPI direct callback으로 자동 fallback
- U1_SFX_FORCE_SDL=1로 M236 경로 강제 A/B 비교 가능
- runtime WAV playback 없음
- SDL_QueueAudio / feeder thread / application PCM queue / idle silence prefill 없음

세부 내용은 ORACLE_NOTES_M237.md와 tools/audit_m237_windows_low_latency.py를 참고하십시오.
