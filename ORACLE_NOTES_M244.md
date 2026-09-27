# M244 — non-invasive WSL audio latency probe

M243 hotfix1 remains the user-validated runtime baseline. M244 changes no default audio transport or PCM generation.

When `U1_AUDIO_PROBE=1`, the Pulse callback records only atomic first-render timing data. Per-sound timing rows are accumulated in memory and written to `audio_probe_m244.csv` on normal shutdown. The probe observes `pa_stream_get_latency()` and `pa_stream_get_timing_info()` before/after the existing M243 source-duration fence.

The unchanged sound(4) PCM begins at about 1.292 ms and reaches |sample| >= 500 at about 1.833 ms, so a perceived three-step lag is not attributable to a long leading-silence segment in the synthesized clip.

No seek, reanchor, playback-completion fence, buffer-size change, resampling, pitch/filter change, or PCM rewrite is introduced.
