# Legacy-folder isolation verification

Verified 2026-10-17. The former Downloads working tree was **renamed**, not deleted:

- Old path: `C:\Users\hojun\Downloads\Ultima1_KR_Work` (does not exist during tests)
- Preserved backup: `C:\Users\hojun\Downloads\Ultima1_KR_Work_BACKUP` (exists)
- Windows: clean CMake+MinGW build from `C:\Users\hojun\Ultima1\snapshot\HF97` into `/tmp/u1_rename_win`, linked `u1kr_clean.exe` and launched it from private isolated runtime; responsive game window confirmed.
- DOS: clean DJGPP build from tracked sources with `U1_DOS_BUILD_DIR=/tmp/ultima1-rename-isolation`, resulting in `ULTIHF99.EXE` with SHA-256 `7e2609e438dfe3fc7487d9015d9475c2c27e06455637b5f03f84388eac7aef95`; DOSBox reported `Program: ULTIHF99`, responsive.
- Independently backed-up files checked by SHA-256 against preserved original: 25 GAMEDATA, 2 FONTS, 4 SAVEGAME; zero mismatches.

Scope: build/link and startup smoke verification only. Does not prove full gameplay regression, source legal redistribution clearance, or independently verify all build dependencies are recoverable without external caches.

**Do not delete** `Ultima1_KR_Work_BACKUP` without the user's explicit separate approval.
