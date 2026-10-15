# HF62 Apple II 1986 direct audio restoration – factual validation (2026-10-09)

## Recovered missing code
Original ProDOS root directory chains block 2 to block 3. Previous extract_1986_remote.py stopped after block 2 and omitted MI.U1.
MI.U1.bin (6589 bytes; aux/load $7000) was extracted from data block 211 in research/ultima1986_u1_program.dsk.
MI.U1 supplies real executable $8266 (20 BC 15 A5 78 60 ...), so SPA $9477 normal flight controller now runs through $15C8, $9497, $8266 and returns normally.
SPA at $8956 overlays the small tail of MI.U1; this is intentional.
C++ controller oracle: 542 calls; 9,075,888 cycles and 8,275 C030 toggles. Exact per-toggle CPU timestamp parity vs corrected py65: mismatches 0.

## Full HyperJump source code executed
CPU started at original SPA $9811 with entry speed 8, persistent MI.U1/SPA/STUPH RAM; stops at $989B after original sound program.
Observed milestones: $9824 acceleration 7 calls; $9868 200; $9886 200; $988D recovery 6; $989B completion.
Fixed erroneous extra ADC/SBC decimal-mode cycles in bundled fake6502 (NMOS 6502 decimal instructions do not take extra clock cycle).
C++ and Python: 7,634,783 cycles, 2,442 speaker toggles, zero individual cycle timestamp mismatches.

## Integrated source prototype, not released
HF62/src/Apple1986Live.cpp loads MI.U1 at $7000; original $9477 normal control and original full $9811 command on hyper request.
Raw virtual speaker output compared with captured local reference research/u1_apple_front_clip.mp4 -> research/apple_front_reference.wav, 7.014 sec.
AppleWin-style one speaker level with inactivity DC attenuation replaces HF61 unverified two-pole low-pass; output 9500 gain.
Original normal first-second 900–1400 Hz spectral power: reference 43.35%, HF62 synthetic RAM 42.29%; band resemblance is encouraging, but NOT proof of bit-exact original game sound.
Standalone CPU->WAV test src/HF62/hf62_live_cpu_audio.wav produced 11 seconds and completed.

## Open release blockers
Visual/game space hyper phase timing in existing SpaceSystem.cpp (DOS-based incremental control) differs significantly from actual 1986 $9811 CPU-executed hyper timing. Full source command sound can continue beyond visual phase or end before it: not acceptable as a confirmed audio restoration.
Actual initialized RAM/game state captured from a running Apple II instance is still absent; direct CPU state uses synthetic seeds.
Need synchronize game visualization with original CPU audio phase events or port stage-driven original code in an objectively verified way.
No user-facing Windows EXE built; deliberately avoiding an immature release per user instructions.

## Reproduction
tools/hf62_walk_prodos.py; tools/hf62_extract_rest.py; tools/hf62_normal_trace.py; tools/hf62_verify_controller_timing.py; tools/hf62_hyper_full_trace.py; tools/hf62_compare_original_hyper.py; tools/hf62_make_wav_harness.py; tools/hf62_compare_reference_bands.py.