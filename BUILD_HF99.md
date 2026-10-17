# HF99 Windows cross-build from GitHub checkout

The repository checkout is the source of truth. Do not build from Downloads/Ultima1_KR_Work. Windows cross-compilation from WSL uses an installed MinGW-w64 compiler and SDL2 MinGW development SDK; SDK/toolchains are external dependencies and are not committed.

## Verified on 2026-10-17

- Windows `u1kr_clean.exe` compiled and linked from `snapshot/HF97` (HF99 source) using WSL CMake and `x86_64-w64-mingw32-g++`.
- The independent build successfully used SDL2 installed outside the old Downloads worktree, at `~/.cache/ultima1-build/SDL2-2.30.0/x86_64-w64-mingw32` (Windows home directory).
- The Linux/WSL application target also compiled successfully. Runtime and packaging validation are separate and still required.
- DOS compilation and linking are now independent of legacy `.o` files. See `BUILD_DOS_HF99.md`. DOSBox runtime validation and packaged delivery are still pending; do not delete the old work directory until acceptance tests pass.

## Windows build example (WSL)

```bash
REPO=/mnt/c/Users/hojun/Ultima1
SDL2_SDK=/mnt/c/Users/hojun/.cache/ultima1-build/SDL2-2.30.0/x86_64-w64-mingw32
cmake -S "$REPO/snapshot/HF97" -B /tmp/ultima1-hf99-win \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DSDL2_DIR="$SDL2_SDK/lib/cmake/SDL2"
cmake --build /tmp/ultima1-hf99-win --target u1kr_clean -j 4
```

The generated EXE is under `/tmp/ultima1-hf99-win/u1kr_clean.exe`. At runtime, SDL2 DLL plus the original Ultima I game data and fonts must be provided separately where required. User savegames and redistributable third-party/game binaries are not part of this source-only Git commit.

## Independent Windows launch

On 2026-10-17, the GitHub-built Windows EXE was launched from `C:\Users\hojun\.cache\ultima1-build\win-runtime` with external SDL2/MinGW DLLs and privately backed-up game data/fonts. The process remained responsive with its game window open; saves in that isolated runtime were empty. This is a startup smoke test, not a full gameplay regression. The source tree under the old Downloads working folder was not used.
