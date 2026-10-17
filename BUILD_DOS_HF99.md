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

## 2026-10-17 CPU compatibility correction

The first source-only DOS executable crashed with `Invalid Opcode` at `0x62301`, which mapped to the `lock cmpxchg8b` instruction emitted for an `std::atomic<uint64_t>` update in `SfxSystem::renderCallback`. The DOS build now compiles baseline DOS objects with `-march=i386` and links a source-controlled single-threaded 64-bit atomic adapter (`snapshot/DOS_support/V1_0K/dos_atomic64_singlethread.cpp`). This adapter is valid only while the DOS audio callback and game run synchronously in one thread; never use it for concurrent callbacks or interrupt handlers.

After rebuilding, DOSBox remained in `Program: ULTIHF99`, screen-present calls appeared in `PRESENT.LOG`, and audio processing advanced beyond 768 blocks in `AUDPERF.LOG`. Actual game screen and input verification remain required.

## Clean rebuild verification

A completely fresh build with `U1_DOS_BUILD_DIR=/tmp/ultima1-hf99-clean-verification` succeeded on 2026-10-17; no cached `.o` files were used. Clean EXE SHA-256: `2fe2a7973d284a196b2810ff6c5f48baa95b7591088c7e6f4885477cb6353cdc`. The clean executable ran under DOSBox and remained in `Program: ULTIHF99` beyond initial boot. `PRESENT.LOG` recorded display calls and `AUDPERF.LOG` recorded audio blocks. This verifies execution progress but does not substitute for visual gameplay inspection.

## DOSBox scaling and window geometry

DOSBox config `snapshot/DOS_support/dosbox_800x600.conf` uses OpenGL output and `windowresolution=800x600`. Run DOSBox with its working directory set to the staged game runtime: `DOSBox.exe -conf <repo>\snapshot\DOS_support\dosbox_800x600.conf`. Window content is 800x600; the measured outer window was 806x629 on Windows.

The 640x400 ARGB source frame is vertically bilinear-resampled to a 640x480 4:3 indexed EGA frame before presentation. Horizontal source and destination resolutions are equal, so no horizontal interpolation is required. Indexed EGA quantization necessarily limits the smoothness of blended colors compared with truecolor Windows rendering. Verified compilation and emulator startup; subjective visual matching requires a human comparison.

## User visual confirmation

User confirmed on 2026-10-17 that DOS palette, bilinear presentation and 800x600 window size look the same as the Windows edition.

## Repository-managed fonts

Project owner confirmed `H12.FNT` and `ROBO.FNT` were created for this project. Both are committed under `assets/FONTS/`. To stage a runtime with privately held commercial game data, run `bash snapshot/DOS_support/stage_runtime.sh OUTPUT_DIR PATH_TO_GAMEDATA`. The script copies the repository fonts and externally provided game data, and creates an empty SAVEGAME directory without transferring any personal saves.
