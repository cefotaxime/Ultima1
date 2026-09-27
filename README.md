# Ultima I Korean Localization — M247 SPACE key completion

M247 keeps the validated M246 Windows-host event-driven WASAPI audio path unchanged.

SPACE input fixes:
- N now toggles the shared Noise state and immediately applies it to SfxSystem.
- Z now opens the existing Ztats/status-inventory presentation.
- While Ztats owns input, continuous SPACE top/front flight timing is frozen, preserving the original pause behavior.
- Existing recovered SPACE controls remain unchanged: arrows, V View, I Sector Scan, H HyperJump, F Fire, front-view speeds 1..8, Space as top-view Pass/front-view Center, and arrow-key dock ship selection.
- Q/save remains unavailable in SPACE; saving is an outside-world-only command.

The full runnable source/package remains the canonical ZIP artifact; this GitHub repository is still the audio-focused subset.
