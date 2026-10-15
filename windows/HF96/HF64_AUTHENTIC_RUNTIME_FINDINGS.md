# HF64 — Authentic Apple II ProDOS runtime capture and sound oracle
Date: 2026-10-09 (KST)

## Important breakthrough
Launched original 1986 Ultima I ProDOS program disk in AppleWin v1.32.1.0. The game really booted, displayed title and character generation screens.
Captured 65536 bytes of *real Apple II emulated running game RAM* using AppleWin debugger F7 and BSAVE "intro_ram.bin",0:0000:ffff.
Snapshot: C:/Users/hojun/Downloads/Ultima1_KR_Work/research/intro_ram.bin.
Verified snapshot 0x8266 instructions match original MI.U1.bin; snapshot also has genuine trampoline table at 0x15C8 and resident code at 0x8078. This is genuine running RAM, but from intro/character creation, NOT from space flight.

## HF64 prototype
Copied HF63 sources into HF64.
New AppleRealRAMSnapshot.h embeds 65536-byte AppleWin captured RAM. Audio reset starts from this memory rather than zero-filled synthetic RAM. The original 1986 MI.U1, SPA and STUPH binary modules are overlaid at the appropriate ProDOS declared load addresses. SPA startup $4e/$4f pointer pair initialized from 0x7eef/$7ef0; $93f9 cleared.
All 6502 code is executed by CPU interpreter with corrected 6502 decimal arithmetic timing.
No new oscillator, synthesized pitch or cutoff added.

## Oracle tests
C++ full source HyperJump $9811, speed8, using AppleWin RAM: 2,271,595 6502 instructions, 7,629,630 cycles, 2,767 C030 speaker toggles. Corrected independent py65 interpreter: identical instruction count, cycle count and 2,767 individual speaker toggle timestamps; 0 mismatches.
HF63 synthetic seed resulted in 2,442 HyperJump toggles: authentic resident game state materially affects emitted sound.
Eight-speed end-to-end audio/visual simulation: All speeds 1..8 complete, all phases seen in proper order, 100 fuel deducted once, craft stays alive.
At speed8, original 6502 HyperJump audio+visual completed ~8.15sec from H-key; speeds3-8 similar. Speed1 takes ~50.57s, speed2 ~29.23s because literal original SPA $9827 DEC speed / $9890 INC speed wrapping at 8bit; do not patch or truncate this original quirk.
Consecutive HyperJump audio test also passed (HF63/HF64).

## First-second Front View audio spectra / actual reference
Reference: research/u1_apple_front_clip.mp4 (7.014 seconds), extracted at 44,100Hz to research/apple_front_reference.wav.
Actual source-generated HF64 WAV: src/HF64/hf62_live_cpu_audio.wav (internal 11-second test). Resynth source is original $9477 and $9811 CPU execution, virtual Apple speaker toggles, amplitude matches application gain 9500.
Reference first-second RMS ~0.2974, HF64 generated first-second RMS ~0.2895.
Eight-frequency-band total percentage-point error (L1) first second: HF62 synthetic RAM ~12.99, HF64 real intro RAM ~12.91. Numeric resemblance is not subjective proof of accurate tone or correct in-space RAM.

## Unresolved fidelity limitations
The saved AppleWin memory is authentic but from character generation, NOT an actual in-space RAM snapshot. Game-specific star random seed, asset pointers and live flags may differ from real SPA/space runtime. We must not label HF64 a fully original-equivalent version on this evidence.
Attempting to run SPA startup from intro RAM in our CPU interpreter eventually stalls at ProDOS disk-service code $7363 since Disk II peripheral is not emulated. A real in-space snapshot or exact ProDOS I/O model is required to complete this step.
HF64 did not generate a new Windows EXE or user-facing package. Only offline tests, source, WAV experiments and research docs are in the project work folder.

## Repro scripts
tools/hf64_authentic_seed.py
tools/hf64_realram_oracle.py
tools/hf64_compare_realram_hyper.py
tools/hf64_score_wave.py
src/HF64/tools/test_all_speeds.cpp
src/HF64/tools/test_low_speed_hyper.cpp
src/HF64/tools/test_normal_speeds.cpp
src/HF64/tools/test_visual_speed_matrix.cpp
src/HF64/tools/test_spa_boot.cpp