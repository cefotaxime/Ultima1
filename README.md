# Ultima I Korean Localization — audio reconstruction branch

Current audio milestone: **M240**

The canonical runtime remains WSL/Ubuntu. M240 preserves the approved M236 hotfix1 PC-speaker synthesis and changes the sparse-effect PulseAudio scheduling model:

- no idle zero-PCM prefill
- playback/read position estimated with `pa_stream_get_latency()`
- logical render clock reanchored to that playback position for each new sound
- first live block written with `PA_SEEK_RELATIVE_ON_READ`
- maximum direct Pulse burst: 240 frames / 5 ms at 48 kHz
- `prebuf=0` + `PA_STREAM_START_CORKED` + one explicit uncork
- existing live filter tails are re-mixed on the reanchored timeline
- SDL remains the fallback

Runtime diagnostics in the full package are written to `audio_runtime_m240.txt`.

Approved synthesis SHA-256:
`0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126`
