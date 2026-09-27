# M245 — Windows-host audio bridge

M244 diagnostics showed that the application reaches the PulseAudio write callback in only a few milliseconds while the WSLg RDPSink reports roughly 0.98 s of sink/stream latency. The approved sound(4) PCM itself begins within about 1–2 ms, so the remaining delay is downstream of the application's PCM generation and direct libpulse client queue.

M245 therefore leaves gameplay in WSL but bypasses WSLg audio when possible. The Linux game launches `tools/u1_audio_host.ps1` through WSL interop, uploads the already-rendered M241-approved 48 kHz mono S16 PCM clips byte-for-byte, and thereafter sends only sound IDs. The Windows companion mixes overlapping clip tails in a host-local waveOut stream using three 128-frame buffers (384 frames = 8 ms explicit queue).

No PC-speaker synthesis, pitch, filter, source duration, read-clock reanchor, PCM seek, or playback-completion fence is changed. If the Windows helper cannot start, M245 falls back automatically to the M243 direct-PulseAudio path. `U1_SFX_FORCE_PULSE=1` forces the fallback for A/B testing.
