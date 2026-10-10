# HF93 — Alakazot successful spell gift SFX restoration

Based on HF92. HF92 changed the town healer bonus-spell message from `이얍!` to `수리수리 마수리!` but forgot the string comparison in `src/main.cpp` that triggers `SfxEvent::PositiveEffect` on a successful Drop Pence. HF93 updates the comparison to the current translation, retaining the Shazam (`짜잔!`) branch.

The bonus spell condition in `LocalSimulation.cpp` remains `rng_.range(1,255)<=16`. The DOS `DosRng::range` function operates on 10-bit values 0..1023 reduced by repeated subtraction of 254, so 68/1024 (=6.640625%) of possible 10-bit values satisfy the condition. This is a possible-output ratio, assuming uniform distribution of those raw 10-bit values; the generator's temporal dynamics could cause different sampled frequencies.

Windows MinGW build successful; package `C:\Users\hojun\Downloads\Ultima1_KR_Work\packages\HF93_ALAKAZOT_SFX_FIX.zip`. In-game SFX playback still pending validation.
