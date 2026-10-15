# HF60: direct Apple II 1986 audio port, work in progress
Policy (2026-10-09): stop creating new 165Hz carriers, oscillator ramps, fixed 64-frame repeating samples, and arbitrary sound blending. Preserve HF59 as listening reference, not a target architecture.
Source baseline: copied from HF59 into dedicated HF60, no edits to HF59/HF58/HF57.
Original data: research/ultima1986/SPA.bin loaded at 0x8956 and STUPH.bin loaded at 0x0800; speaker at $C030.
Known STAR update entry: $9497. Apple HyperJump control routine near $9811..$98A0; changes memory at $93F4, calls $9497; accelerate until $93F4==1; two 200-pass loops and return to original speed.
Port required:
- a stateful 6502 execution core (no resetting CPU RNG/star state for each update);
- original executable memory and initialized RAM, mapping original game state and speed;
- cycle-exact speaker $C030 toggle events;
- 1,023,000Hz reference clock -> host sample rate using fractional accumulator;
- speaker output and measured transfer characteristic with no separate invented oscillators;
- deterministic cross-language oracle comparing C++ toggle timestamp streams with py65 traces;
- actual Windows audio regression + EXE packaging.
Initial dependency audit: tools/audit_1986_spa_state_dependencies.py recorded 494 speaker toggles over mixed-speed calls and broad RAM/stack accesses. This is exploratory: code address exclusion needs refinement because executable auxiliary routines are outside SPA/STUPH regions.
No HF60 EXE should be labeled finished before direct 6502 core and oracle are implemented.