# M247 — SPACE Noise / Ztats / key-dispatch completion

The M246 SPACE dispatcher consumed keyboard input before the normal OUT dispatcher and omitted two common Ultima I commands: N (Noise) and Z (Ztats).

M247 adds N as the same shared noiseEnabled toggle used elsewhere and applies it through SfxSystem::setEnabled(). It adds Z as the existing Ztats/inventory presentation. While Ztats is visible, SPACE flight timing is frozen so the status pages also pause the game.

Existing recovered SPACE-specific controls are unchanged: arrow steering/aim, V View, I Sector Scan, H HyperJump, F Fire, front-view 1..8 speed, Space as top-view Pass/front-view Center, and arrow-key dock ship selection. Quit/save remains outside-world-only.

M246 audio synthesis, approved PCM, Windows-host bridge, and event-driven WASAPI path are unchanged.
