# HF60 audio failure audit — 2026-10-09
Listening report: no high+low in normal flight; crackling throughout; momentary high sound near lightspeed.
Code audit: tools/hf60_audio_structure_audit.py
1. Apple1986Live.cpp runs 6502 $9497 star update repeatedly at 1.023MHz, not the game frame scheduler. Initialization is synthetic zero-filled RAM.
2. SDL outputs one raw virtual speaker level with DC removal and gain 4000. It does not model the Apple II physical speaker transfer.
3. main.cpp explicitly suppresses SpaceFlightPulse in all front-view phases; the HF59 normal-flight low layer was thus removed.
4. Exact 10577-event timing equality was against py65 with identical synthetic starting state, not real Apple II runtime.
Decision: Keep HF60 as CPU prototype only, do not ship a new fake-fidelity EXE. Recover original game loop and actual RAM/speaker state; validate normal flight before HyperJump.