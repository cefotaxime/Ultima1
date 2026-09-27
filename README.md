# Ultima I Korean Localization — audio reconstruction branch

Current audio milestone: **M244 diagnostic**

M243 hotfix1 remains the user-validated runtime baseline: correct sound character, one movement sound per movement input, normal player pacing, with roughly three movement commands of perceived WSL audio lag remaining.

M244 does not change the default playback path. With `U1_AUDIO_PROBE=1`, it records read-only per-sound timing observations into `audio_probe_m244.csv` on normal shutdown:

- `play()` begin/end timestamps
- first Pulse render callback timestamp and frame count
- `pa_stream_get_latency()` before/after the existing M243 source-duration fence
- Pulse sink/transport latency plus write/read indexes
- reconstructed PCM onset positions

The real-time callback performs no file I/O and M244 adds no seek, reanchor, playback-completion wait, buffer-size change, resampling, pitch/filter change, or PCM rewrite.

The approved `PcSpeakerEmulator.cpp` SHA-256 remains:
`0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126`
