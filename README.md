# Ultima I Korean Localization — M246 host WASAPI bridge

M245 proved that bypassing WSLg/RDPSink removes the perceptible audio delay, but the legacy Windows `waveOut` backend changed the audible character of the approved PC-speaker PCM.

M246 keeps the successful WSL→Windows host bypass but replaces `waveOut` with event-driven WASAPI. The Windows companion first requests exclusive 48 kHz stereo 16-bit PCM at the endpoint's minimum device period. The approved mono sample value is duplicated to left/right without resampling, filtering, gain scaling, or PCM regeneration. If exclusive mode is unavailable, the helper falls back to event-driven shared WASAPI; if the helper cannot initialize at all, the game falls back to the M243 direct-PulseAudio path.

`audio_runtime_m246.txt` records whether the helper actually opened `wasapi-exclusive` or `wasapi-shared`. Set `U1_SFX_FORCE_PULSE=1` for comparison with the M243 Pulse path.
