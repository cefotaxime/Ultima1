# Ultima I Korean Localization — audio reconstruction branch

Current audio milestone: **M241 rollback**

M240 is rejected by user validation: the PC-speaker sound character changed and audible onset regressed from about 3 steps to about 6 steps.

M241 restores the complete M238 hotfix1 audio implementation. There is no new audio timing experiment in this milestone.

Locked behavior:
- canonical runtime: WSL/Ubuntu
- direct PulseAudio path from M238 hotfix1
- 5 ms requested Pulse latency
- no M239/M240 playback-read-clock reanchor
- no PA_SEEK_RELATIVE_ON_READ scheduling
- PC-speaker synthesis remains the approved M236 hotfix1 model

The next latency experiment must branch from this verified-good audio baseline rather than M239/M240.
