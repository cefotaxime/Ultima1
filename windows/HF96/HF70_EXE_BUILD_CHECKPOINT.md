# HF70 Windows EXE checkpoint — 2026-10-09

- Real AppleWin original Ultima I spacecraft Front View H-key was injected with `fast_apple_keys.py h` and visually verified at `HyperJump... light speed!`, then `HyperJump complete.` / regular Front View return.
- The earlier work only replayed RAM offline; this is the first actual live H-key execution observed in the current continuation.
- Windows build: copied HF66 CMakeLists.txt to HF67 (does not change HF67 gameplay code). Configured cross-compilation with WSL mingw-w64 and SDL2 2.30.0. `cmake --build ... --target u1kr_clean -j 8` completed 100% with exit code 0.
- EXE: `build/HF67_win64/u1kr_clean.exe` (2,408,241 bytes).
- Local smoke environment: extracted prior HF57 package into `validation/HF67_windows_build_smoke` and replaced `Ultima1KR.exe` with freshly built HF67 EXE. SDL2.dll/FONTS/GAMEDATA kept. With SDL_VIDEODRIVER=dummy and SDL_AUDIODRIVER=dummy, executable remained alive 3 seconds (manually stopped). This is a process-liveness check, NOT a gameplay/audio test.
- No audible apples-to-apples synchronized hardware recording has been captured. The HF67 audio defect reported by user (missing upper pitch before light-speed, incorrect rough lower pitch during light-speed) is NOT certified fixed. Do not label this a production hotfix. Main game files not overwritten.