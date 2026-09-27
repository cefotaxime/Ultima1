# Ultima I 한글화 clean-room — M245

M243 hotfix1의 게임 로직과 승인된 PC-speaker PCM은 그대로 유지한다. M244 계측에서 WSLg RDPSink가 약 0.98초의 sink latency를 보고했고, 앱에서 PulseAudio로 첫 데이터를 쓰는 시간은 수 ms 이내였다. 따라서 M245는 WSLg/RDP 오디오 경로를 우회한다.

기본 WSL 실행 시 M245는 Windows PowerShell companion `tools/u1_audio_host.ps1`을 시작한다. 게임은 승인된 48 kHz 16-bit PCM clip 10개를 시작 시 companion으로 그대로 전달하고, 이후 효과음 ID만 보낸다. Windows companion은 host-local waveOut mixer로 128-frame × 3 buffer를 유지하면서 clip tail을 서로 겹쳐 mix한다. PCM을 재생성하거나 pitch/filter/source duration을 바꾸지 않는다.

companion 시작에 실패하면 M243 direct PulseAudio 경로로 자동 fallback한다. 비교를 위해 `U1_SFX_FORCE_PULSE=1`을 지정하면 Windows host bridge를 건너뛰고 기존 Pulse 경로를 강제할 수 있다.

M239/M240의 read-clock/reanchor/seek 방식과 M242 playback-completion fence는 계속 금지한다.
