#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
CXX="$PWD/tools/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++"
OUT=dos_port/DOS_AUDIO_FIX_20261011
OLD=dos_port/HF93_OBJECT_AUDIT
mkdir -p "$OUT/obj" "$OUT/game"
flags=(-std=gnu++17 -O1 -ffunction-sections -fdata-sections -Idos_port/V1_0K -Isrc/HF93/src -include dos_port/V1_0K/dos_singlethread_mutex.h)
for name in dos_hf93_audio_diagnostic dos_hf93_platform dos_sb16_dma; do
 "$CXX" "${flags[@]}" -c "$OUT/source/$name.cpp" -o "$OUT/obj/$name.o"
done
objects=()
for obj in "$OLD"/*.o; do
 case "$(basename "$obj")" in
 HF93_main.o|HF97_main_port_check.o|FrontendSystem.o|dos_hf93_audio_diagnostic.o|dos_hf93_platform.o|dos_sb16_dma.o) continue;;
 esac
 objects+=("$obj")
done
"$CXX" -Wl,--gc-sections "${objects[@]}" "$OUT"/obj/*.o -o "$OUT/game/ULTISND.EXE"
# Separate harness links the same production audio objects and original SFX mixer.
"$CXX" "${flags[@]}" -Wl,--gc-sections "$OUT/source/audio_test.cpp" "$OUT"/obj/*.o \
 "$OLD/SfxSystem.o" "$OLD/SfxPcmM212.o" "$OLD/SfxPcmOracleM225.o" "$OLD/PcSpeakerEmulator.o" \
 "$OLD/Apple1986Live.o" "$OLD/fake6502.o" "$OLD/dos_atomic64_singlethread.o" -o "$OUT/game/AUDTEST.EXE"
echo BUILD_OK