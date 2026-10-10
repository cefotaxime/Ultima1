# Ultima I Korean DOS Port — Handoff (2026-10-10)

Status: **partial DOS prototype**, not completed game. Full Korean Windows implementation lives in local `Ultima1_KR_Work/src/HF93/src`; DOS prototype lives in local `Ultima1_KR_Work/dos_port/V1_0K`.

Validated: DOSBox field map, cardinal movement, water/mountain collision consuming one turn, Korean collision bitmaps, `sound(0)`/`sound(4)` captured PCM routed to 1-bit PC speaker. Four regression tests pass: MOVE SOUND, COLLISION, WATER, SAVE (3 moves). Sound timing approximately 76.97ms and 28.65ms, but perceptual waveform parity unverified. Full gameplay, cities, castles, dungeons, combat, spells, space and ending remain unported.

On the user's Windows PC, `dos_port/BUILD_TEST_PACKAGE.ps1` builds with WSL DJGPP, tests in DOSBox, and packages `ULTIMA1_KR_DOS_VERIFIED_HANDOFF.zip`. `dos_port/PACKAGE_DOS.py` packages without rebuilding. Two local recovery packages: `ULTIMA1_KR_DOS_VERIFIED_HANDOFF.zip` (DOS prototype/control scripts) and `ULTIMA1_KR_SOURCE_RECOVERY.zip` (HF93 and DOS C++ sources). **These packages have not been uploaded to GitHub.** The main project workspace itself is not a Git repository as of this checkpoint.

Do not overwrite original `PLAYER1.U1`; export candidate saves to `PLAYER1.NEW`.

To resume: restore full workspace and tools (WSL DJGPP, DOSBox 0.74-3), run Windows PowerShell `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "C:\Users\hojun\Downloads\Ultima1_KR_Work\dos_port\BUILD_TEST_PACKAGE.ps1"`. Verify four PASS lines and package SHA. Keep proprietary assets separate from source-control uploads.
