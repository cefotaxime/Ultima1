# HF60 original-code speaker oracle progress — 2026-10-09
HF60 independent source exists; HF59 unchanged.
New harness: C:\Users\hojun\Downloads\Ultima1_KR_Work\tools\hf60_live_6502_oracle.py
Input: original 1986 SPA.bin @ $8956, STUPH.bin @ $0800; py65 cycle-accounting CPU.
542 stateful calls to Apple routine $9497; 8,939,882 CPU cycles and 10,577 speaker toggles at $C030.
Script preserves memory, PRNG seed progression and starfield state across calls, and restarts only entry PC+stack sentinel per call.
It does NOT use HF58/59 repeating 64-call event tables, nor introduce an independent low oscillator.
Synthetic flight speed schedule used in test: normal speed 8 x64, descend through 7..1, then speed1 x400, ascend through2..8, normal8 x64.
This IS a live CPU-interpretation proof for a partial Apple code path; it is NOT yet the fully authentic 1986 HyperJump command, not yet a Windows/SDL integration, and not evidence of accurately initialized original boot RAM.
Next implementation gate: C++ 6502 executable with cycle/toggle oracle equality against the Python run; run genuine Apple HyperJump speed scheduler, audio buffer mapping, and verify output without periodic clicks.