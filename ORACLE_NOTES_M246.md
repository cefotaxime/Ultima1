# M246 — Windows host event-driven WASAPI

M245 user validation established that the WSLg bypass eliminates perceived delay while the legacy host waveOut path changes audible character.

M246 preserves the host bypass and exact M241-approved 48 kHz mono S16 clips, but replaces waveOut with event-driven WASAPI. Exclusive 48 kHz stereo S16 at the endpoint minimum period is attempted first; the mono sample is duplicated L/R exactly. Event-driven shared WASAPI is fallback, followed by the M243 direct-Pulse path if host initialization fails.

No synthesis, resampling, pitch/filter, source-duration, read-clock reanchor, PCM seek, or playback-completion fence is introduced.
