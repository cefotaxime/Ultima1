# HF99 DOS source-only build

Verified on 2026-10-17: source code checked out under `C:\Users\hojun\Ultima1` compiled and linked into a DJGPP MZ/COFF DOS executable without linking any of the 34 historical `HF93_OBJECT_AUDIT/*.o` files and without reading the old Downloads working source tree.

## Build (WSL)

Install the DJGPP GCC cross-toolchain independently of this repository. Set `DJGPP_CXX` to the `i586-pc-msdosdjgpp-g++` executable (or use the default cached path on the development PC). Build outputs are written under `/tmp/ultima1-dos-source-check` by default, which can be overridden with `U1_DOS_BUILD_DIR`.

```bash
cd /mnt/c/Users/hojun/Ultima1
bash snapshot/DOS_support/build_full_from_source.sh
file /tmp/ultima1-dos-source-check/ULTIHF99.EXE
```

The script first builds the baseline modules, DOS SDL compatibility layer, audio backend and C-based `fake6502` interpreter. It then compiles the HF99 DOS entry point and latest restored dungeon/overworld code and links all freshly generated objects.

Verified binary SHA-256 (initial 2026-10-17 source-only link): `d9d7d23d42a8672569fcf2fd6439ddfa3dd33d1bf8de18fb5018de59185bd2b3`. Subsequent changes can alter this hash.

## Separate assets and runtime

Original Ultima I `GAMEDATA`, fonts and user `SAVEGAME` are NOT stored on GitHub. The current PC has checked and hashed private backup copies under `C:\Users\hojun\.local\share\Ultima1_KR`. The isolated smoke-test runtime is under `C:\Users\hojun\.cache\ultima1-build\dos-runtime`; its `SAVEGAME` folder begins empty. To test the DOS EXE, stage the compiled `ULTIHF99.EXE`, the legally owned `GAMEDATA` and `FONTS` directories, and the DJGPP `CWSDPMI.EXE` dependency in a temporary DOSBox mount.

`DOSBox 0.74-3` was launched with the independently linked executable, and startup wrote `WINDOW.LOG` and `AUDIO_RU.TXT`. GUI/video/audio gameplay correctness has NOT yet been visually validated; the process start and log creation are insufficient to claim full functionality.

## Remaining acceptance gates

- Visual DOSBox startup and gameplay confirmation with independent executable.
- Windows native executable launch and packaged asset/dependency smoke check.
- All private saves verified separately and retained before removing old working tree.
- Temporary rename of old working tree with successful Windows + DOS rebuild and launch, then ask user to approve deletion.
