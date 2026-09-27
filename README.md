# Ultima I Korean Localization — M245 host-audio bridge

M245 preserves the M243 hotfix1 gameplay pacing and the approved M236/M241 PC-speaker PCM, but bypasses the WSLg PulseAudio/RDP sink for sound output.

Canonical WSL launch now tries a small Windows PowerShell companion first. The Linux game uploads the already-rendered 48 kHz mono S16 clips byte-for-byte and then sends only sound IDs. The Windows helper mixes overlapping tails with three 128-frame host-local waveOut buffers (8 ms of explicit application-side queue). No PCM regeneration, resampling, pitch/filter change, read-clock reanchor, or playback-completion fence is added.

If the host helper cannot start, M245 automatically falls back to the M243 direct-PulseAudio path. Set `U1_SFX_FORCE_PULSE=1` for explicit A/B comparison.
