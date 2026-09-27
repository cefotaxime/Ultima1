# Ultima I Korean Localization — audio reconstruction branch

Current audio milestone: **M243 rollback**

M242 is rejected by user validation. One movement input could produce two or three footstep-like sounds, and player action pacing became visibly slower.

M243 restores the exact M241 hotfix1 audio core. There is no new latency experiment in this milestone.

Locked behavior:
- canonical runtime: WSL/Ubuntu
- M241 hotfix1 SfxSystem restored exactly
- approved M236 hotfix1 PC-speaker synthesis unchanged
- no M242 playback-completion fence
- no M239/M240 Pulse read-clock reanchor or PA_SEEK_RELATIVE_ON_READ
- no PCM rewrite/resampling/timing reinterpretation

Future latency work must not block the game loop on downstream playback completion.
