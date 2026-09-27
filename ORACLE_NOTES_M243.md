# M243 — reject M242 playback fence

User validation of M242 found a clear regression: one movement input could sound like two or three footsteps and player actions became slower.

M243 removes the M242 playback-completion fence and restores the exact M241 hotfix1 SfxSystem implementation. The audio synthesis and direct PulseAudio path are unchanged from the previously user-validated baseline.

Rejected for future baseline use: blocking play() on downstream Pulse playback completion, PA_SEEK_RELATIVE_ON_READ reanchor logic, and any PCM timeline reinterpretation.
