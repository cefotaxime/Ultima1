# DOS 2026-10-17 runtime verification

- Confirmed by manual user test: `ret_fast.conf` using `cycles=max` gives responsive front-view flight and correct alien kill/erase.
- Previous `ret.conf` with `cycles=12000` caused severe lag in the same scenario. Preserve it for comparison only.
- `ret_nosound.conf` intentionally disables SB16 and produces DSP reset failure; it is a negative-control test, NOT a launch configuration.
- Sound Blaster test settings: sbtype=sb16, sbbase=220, irq=7, dma=1, hdma=5; BLASTER=A220 I7 D1 H5 T6.
- `main.cpp` headless alien-hit diagnosis now decrements `alienShotFrames` because ordinary game presentation does so; rebuilt with successful compile/link.
- `RET.BAT` launches a diagnostic (`--skip-frontend --test-alien-hit`), not a normal release game. Do not mistake the resulting front view for complete game validation.
- Automated DOSBox capture: MENUCHK.BAT completed START / OK / RETURN and produced 640x400 MENUCHK.BMP; image contents require visual comparison.
- Automated Space test capture: SPACECHK.BAT with `--test-space-hit --capture-once` completed START / OK / RETURN and produced 640x400 SPACECHK.BMP; image contents require visual comparison.
- User manual test confirmed new character creation, 820-byte save to SAVEGAME/PLAYER1.U1, launching that character into gameplay, and automatic initial Shift-H command help. Save SHA256 D9C2A095621718BDBE279080298FEF20A24CF81CBBA67AE3D33AC037237FA5DB; independent test copy TESTS/PLAYER1_OK_20261017.U1.
- Automated FIELDCHK.BAT using independent save copy completed START/OK/RETURN, 640x400 BMP with 11 colors and full-frame nonblack bounds.
- DOCKTEST.LOG reports seven PASS checks, including landing, docking charge, replenishment, undocking and non-shuttle fatal collision.
- MONDAUD.LOG reports eight PASS checks, including beep delay, dissolve/scroll timing and restart after 5 seconds.
- Further user manual verification: field movement and city/castle entry, dungeon entry, melee kill, combat sounds, chest/coffin opening, Prayer success/failure sound all passed. Light Sword has range 1 per original OUT.EXE DS:26DE; user verified ranged weapon (Phazor/Blaster) correctly attacks enemies at distance 2-3 in DOSBox.
- Further validation needed: remaining magic spells, save/load persistence through multiple sessions, Space/Mondain audiovisual behavior, and sound/timing comparison with the original.
- Preserve Windows version, original saves and all source branches.