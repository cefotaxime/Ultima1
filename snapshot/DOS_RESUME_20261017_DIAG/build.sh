#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/hojun/Downloads/Ultima1_KR_Work
d=dos_port/DOS_RESUME_20261017_DIAG
cxx=tools/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++
flags=(-std=gnu++17 -O1 -ffunction-sections -fdata-sections -I"$d/source" -Idos_port/V1_0K -Isrc/HF93/src -include dos_port/V1_0K/dos_singlethread_mutex.h)
for n in main SpaceSystem; do
 "$cxx" "${flags[@]}" -c "$d/source/$n.cpp" -o "$d/$n.o" > "$d/$n.compile.log" 2>&1
 echo "COMPILE_OK $n"
done
"$cxx" "${flags[@]}" -Isrc/HF97/src -c src/HF97/src/DungeonSystem.cpp -o "$d/DungeonSystem_HF97.o" > "$d/DungeonSystem.compile.log" 2>&1
echo COMPILE_OK DungeonSystem_HF97
objects=()
for obj in dos_port/HF93_OBJECT_AUDIT/*.o; do
 case "$(basename "$obj")" in
 HF93_main.o|HF97_main_port_check.o|HF97_DOS_CP949_main.o|FrontendSystem.o|SpaceSystem.o|DungeonSystem.o|dos_hf93_audio_diagnostic.o|dos_hf93_platform.o|dos_sb16_dma.o)continue;;
 esac
 objects+=("$obj")
done
mkdir -p "$d/game" "$d/game/SAVEGAME"
"$cxx" -Wl,--gc-sections "$d/main.o" "$d/SpaceSystem.o" "$d/DungeonSystem_HF97.o" "${objects[@]}" dos_port/DOS_AUDIO_FIX_20261011/obj/dos_hf93_audio_diagnostic.o dos_port/DOS_AUDIO_FIX_20261011/obj/dos_hf93_platform.o dos_port/DOS_AUDIO_FIX_20261011/obj/dos_sb16_dma.o -o "$d/game/ULTIDIAG.EXE" > "$d/link.log" 2>&1
echo LINK_OK
sha256sum "$d/game/ULTIDIAG.EXE"