# Ultima I Korean Localization — audio reconstruction branch

Current audio milestone: **M242**

User-validated baseline is M241 hotfix1: correct PC-speaker sound character with about three movement commands of audible lag under the canonical WSL runtime.

M242 changes only the synchronous return fence:
- PC-speaker synthesis is unchanged
- direct PulseAudio write callback and 5 ms latency request are unchanged
- no PCM seek, rewrite, reanchor, or resampling
- no PA_SEEK_RELATIVE_ON_READ
- after the existing rendered-sample fence, the WSL path waits for the measured Pulse playback/read cursor to cross the same source-end sample
- if Pulse timing is unavailable, it immediately falls back to M241 behavior

M239/M240 remain rejected as baselines because they changed sound character.
