# Apple II 1986 original SPA callgraph (2026-10-09)

Extracted from SPA.bin loaded at $8956; verified by byte scan + py65 disassembly.

## Normal flight
- $9477: JSR $15C8 (resident controller/input/update), JSR $9497 (star frame), JSR $8266, conditional RTS or JMP $15BF.
- $9486: LDA #$32; STA $9496; JSR $9477; loop with DEC $9496. This also requires investigating the actual entry mode rather than assuming every frame takes this path.
- $9497 is a subroutine, not a self-scheduling audio thread. HF60 currently calls it back-to-back at CPU rate, losing host/game controller and update pacing.

## HyperJump
- $9811 stores entry speed $93F4 into $98F1; resets $93F9 and $93FA.
- $9824 repeatedly JSR $9497; DEC $93F4 until speed == 1.
- $9865 JSR $9452, which initializes countdown $BC=$C8 (200).
- $9868 invokes $9497 200 times.
- $9880 JSR $9452; $9883 INC $93F4; $9886 invokes $9497 200 times.
- $988D invokes $9497 then INC $93F4 until speed == entry speed $98F1.
- $9497 contains $C030 toggles at $9510, $9539 and $959F; expected timings depend on RAM, flags, position and resident helper routines.

## Findings and remaining barriers
1. Distinct call sites: normal $947A, hyper $9824/$9868/$9886/$988D. The direct live model is not faithful without this controller.
2. Apple 6502 cycle oracle was proven only for synthetic RAM and manual speed schedule, not this actual call graph.
3. Original hardware synchronization and screen/video RAM state are not recovered. Do not add an arbitrary 70 Hz delay to fake correctness.
4. Next: trace $9477/$15C8/$8266/$15BF with actual game state and distinguish which CPU execution paths contain $C030 toggles before modifying live audio output.

Supporting tools: tools/hf60_scan_original_star_callers.py and tools/hf60_disasm_callers.py.
No HF61 build is authorized by evidence yet; existing HF60 retains its prototype status.