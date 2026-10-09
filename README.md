# Ultima I Korean Localization — HF75

Current Windows build source: [HF75/](HF75/). Earlier M262 content remains below as historical release notes.

## HF75 sound changes (2026-10-09)
- Dungeon: Apple II behavior. An attack when no monster is in range prints the existing no-target message **without attack-launch sound**. Attacks against an actual monster retain the existing attack/hit sound behavior.
- Space Top View: successful **Thrust** and **Retro** inputs play the **same OutdoorLandMove sound event** used for ordinary field foot movement, per project approval. Failed moves, ship-selection controls and Front View reticle controls do not trigger this sound.
- Previously implemented Front View / HyperJump audio is unchanged.

## Windows build
The main source is under `HF75/src/` with `HF75/third_party/fake6502/`. Build the `u1kr_clean` target using a C++17 toolchain and SDL2 CMake package. Example for the project's WSL MinGW setup:

```bash
cmake -S HF75 -B build/HF75_win64 \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=/usr/bin/x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/x86_64-w64-mingw32-g++ \
  -DSDL2_DIR=/path/to/SDL2/x86_64-w64-mingw32/lib/cmake/SDL2
cmake --build build/HF75_win64 --target u1kr_clean -j4
```

Package the resulting `u1kr_clean.exe` as `Ultima1KR.exe` alongside the existing licensed game data, fonts and SDL2 runtime DLLs. Original game assets are not included in the source tree.

## History

# Ultima I Korean Localization — M262

M262 restores the original light-cyan text color for the framed MONDAIN popups shown before and after the final encounter. The four Time Machine narration pages, the victory popup, and the defeat popup now use EGA light cyan. The RGBW labels on the first Time Machine page follow the same popup text color; the four BIOS gem glyphs retain their original red/green/blue/white colors.

Historical M256 was updated so it no longer pins the obsolete white popup text implementation.
