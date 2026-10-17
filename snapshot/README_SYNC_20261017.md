# Ultima I Korean - Windows HF97 / DOS port synchronization (2026-10-17)

This snapshot preserves the local Windows HF97 source tree (64 files) and the current DOS integrated port's text sources, supporting DOS compatibility/audio source, build script, configuration and selected audits. It does not replace the previously committed HF75-HF93 work.

## Layout
- `snapshot/HF97/src/`: source files copied from local `Ultima1_KR_Work/src/HF97/src/`.
- `snapshot/HF97/CMakeLists.txt`: local HF97 CMake project.
- `snapshot/DOS_RESUME_20261017_DIAG/`: latest integration source, `build.sh`, runtime notes, and DOSBox configs.
- `snapshot/DOS_support/V1_0K/`, `snapshot/DOS_support/audio/`: DOS compatibility layer and SB16 audio sources.
- `snapshot/DOS_support/audits/`: selected source-code regression checks.

## Important limitations
The current DOS build script uses precompiled `dos_port/HF93_OBJECT_AUDIT/*.o` and proprietary/local toolchain/data paths. The binary object files, DJGPP compiler, original game assets, save files, logs, and test screenshots are deliberately **not** committed. The repository is a **source snapshot**, not a standalone reproducible DOS build or validated release. Check and regenerate the precompiled objects against HF97 before making a new DOS release.

## Known DOS regression
The active DOS executable was linked against a stale `DungeonSystem.o` built before the corrected ladder spell mapping landed in HF93. The correct HF97 mapping: spell 5 = ASCENDO (ladder up, feature 7), spell 6 = INFERUS (ladder down, feature 6). The observed pre-fix DOS executable creates an up ladder when casting INFERUS. This snapshot **does not fix the executable**; rebuild the DOS dungeon object from the current source, relink, then test Cast + Klimb for both spells.

## Validation
Earlier user-verified functionality: DOS main menu, character save, field movement, castle/town entry, dungeon exploration, chest/coffin, melee/ranged combat and sounds, Prayer effects. Ladder spell routing remains a confirmed open issue. Do not treat passing source-level audits as DOSBox runtime verification.
