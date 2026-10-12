# Ultima I Korean Windows HF94 — Mondain burn / defeat popup

Status: **compiled; gameplay verification pending** (2026-10-12).

Local authoritative paths:
- Source: `C:\Users\hojun\Downloads\Ultima1_KR_Work\src\HF94`
- EXE: `C:\Users\hojun\Downloads\Ultima1_KR_Work\build\HF94_win64_clean\u1kr_clean.exe`
- Setup: `C:\Users\hojun\Downloads\Ultima1_KR_Work\distribution\HF94_Mondain_Setup\Ultima1_Korean_HF94_Mondain_Test_Setup.exe`
- Installer SHA256: `F39FFD96526595A4FB612A07ABD8F5AA10D9C2A51147BF688CF6094A0442DD9F`
- Game EXE SHA256: `bc9136cedf662395137957a1830707a58da403fed59ab2627d6212a3ae4f71cf`

Behavioral requirements:
1. All four collision targets (wall, gem, Mondain, arena boundary) burn: `max(1, hp_before/10)` with integer truncation; message `화상을 입습니다! N의 피해량!`.
2. Lethal collision sets `mondain.defeated` and death message.
3. Defeat popup ignores key input, hides command cursor, and auto-closes after 50 BIOS ticks (~2747ms), followed by existing skull/resurrection sequence.
4. User save files untouched; test installer installs under distinct `U1K_Win_HF94_Test` directory.

Compilation: MinGW-w64 / CMake target `u1kr_clean`, PASS. Setup C# embedded resources `setup.ps1` and `payload.zip` verified. Actual arena collision/death timing is **NOT YET verified by human gameplay**.

The full local source must still be synchronized with GitHub from the PC (do not claim this note is a source backup). The main branch has not been modified.
