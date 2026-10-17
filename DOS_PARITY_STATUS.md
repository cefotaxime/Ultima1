# DOS HF99: Windows parity corrections

Reference: current Windows `snapshot/HF97/src` and original HF94–HF97 migration records in the preserved legacy backup.

| Feature | Current DOS status | Evidence |
|---|---|---|
| Mondain collision: damage, health reduction, localized burned message, inverse frame, sound(0)/sound(2) | Ported to current DOS source; DJGPP build/link and emulator startup passed. In-game collision still requires a gameplay check. | `snapshot/DOS_RESUME_20261017_DIAG/source/main.cpp` and `snapshot/HF97/src/main.cpp` |
| Mondain defeat transition using 50 BIOS ticks | Not yet ported. | `apply_hf94_mondain.py`; `mondainDefeatPopupStartedMs` present in Windows but absent in DOS |
| Food purchase quantity zero/invalid key cancellation | Not yet ported. | `apply_hf97_food_cancel.py`; DOS build uses HF93 `ShopSystem.cpp` |
| Dungeon lethal visual presentation timing | Present in DOS entry point; gameplay check pending. | `dungeonLethalPrePresent` in DOS entry point |

The DOS executable currently mixes modules from HF93, HF97 and DOS-specific resume sources. Changes are made only to the current HF99 source; do not restore or build historical versions as a replacement for HF99.

Validation requirement: build/link success and DOSBox startup are not substitutes for a gameplay-level verification. No full parity claim is made.

## 2026-10-17: Mondain defeat popup regression

- Reported regression: while the player-death popup is shown, a new command line and cursor become visible in DOS.
- Historical evidence: `apply_hf94_mondain.py`, Windows `main.cpp` timer/keyboard guard and log cursor guard.
- DOS HF99 correction: ignore input during the popup; keep it for 50 BIOS ticks (~2747 ms), automatically acknowledge and run resurrection; suppress command-log cursor and command-entry overlays during defeated state.
- New DOS build/link and emulator startup passed. In-game death-popup validation remains outstanding.
