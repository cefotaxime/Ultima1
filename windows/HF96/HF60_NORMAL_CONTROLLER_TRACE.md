# HF60 normal-flight controller trace — 2026-10-09
Source: 1986 ProDOS SPA.bin and STUPH.bin; synthetic memory initialized as earlier audio oracle.
Executed CPU at $9477, preserving original 6502 opcode timing (DEC absolute CE = 6 cycles).
Observed execution path: $9477 (JSR $15C8) -> $947A (JSR $9497) -> $947D (JSR $8266).
$15C8 returns to SPA after about 330 cycles; $9497 returns after about 24,441 cycles; at $8266 execution falls into uninitialized/zero RAM and test never returns (PC=$0000 after 150000 steps).
SPA.bin is loaded at $8956 through $B020; STUPH.bin from $0800 through $2000. Neither range covers $8266. Thus existing CPU harness omits a resident or loader-provided code segment needed for the actual normal-flight controller.
Original ProDOS disk image remains at research/ultima1986_u1_program.dsk; extract_1986_remote.py previously extracted only catalog files, not complete runtime memory initialized by the boot/loader.
Next: reconstruct actual 1986 loader/runtime memory (including $8266) before audio code modifications; trace $9477 until actual return, then compare actual Apple-speaker toggle cadence.
No change to audio renderer, EXE, or HF59. This is an evidence-only trace. Script tools/hf60_normal_controller_trace.py.