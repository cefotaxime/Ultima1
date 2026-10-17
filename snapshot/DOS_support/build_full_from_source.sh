#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/snapshot/DOS_support/HF93_src"
PORT="$ROOT/snapshot/DOS_support/V1_0K"
UPDATED="$ROOT/snapshot/HF97/src"
RESUME="$ROOT/snapshot/DOS_RESUME_20261017_DIAG/source"
OUT="${U1_DOS_BUILD_DIR:-${TMPDIR:-/tmp}/ultima1-dos-source-check}"
CXX="${DJGPP_CXX:-/mnt/c/Users/hojun/.cache/ultima1-build/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++}"
"$ROOT/snapshot/DOS_support/build_objects_from_source.sh"
flags=(-std=gnu++17 -O1 -ffunction-sections -fdata-sections -I"$RESUME" -I"$PORT" -I"$SRC" -include "$PORT/dos_singlethread_mutex.h")
compile() {
 local file="$1" name="$2"
 echo "COMPILE $name"
 "$CXX" "${flags[@]}" -I"$UPDATED" -c "$file" -o "$OUT/$name.o" >"$OUT/$name.log" 2>&1 || { tail -40 "$OUT/$name.log"; echo "COMPILE_FAILED:$name"; exit 1; }
}
compile "$PORT/dos_sdl_video.cpp" dos_sdl_video
compile "$SRC/FrontendSystem.cpp" FrontendSystem
compile "$RESUME/main.cpp" main
compile "$RESUME/SpaceSystem.cpp" SpaceSystem
compile "$UPDATED/DungeonSystem.cpp" DungeonSystem
compile "$RESUME/OriginalData.cpp" OriginalData
compile "$UPDATED/OverworldSimulation.cpp" OverworldSimulation
objects=("$OUT"/*.o)
echo 'LINK DOS'
"$CXX" -Wl,--gc-sections "${objects[@]}" -o "$OUT/ULTIHF99.EXE" >"$OUT/link.log" 2>&1 || { tail -50 "$OUT/link.log"; echo 'LINK_FAILED'; exit 1; }
echo 'DOS_SOURCE_LINK_OK'
sha256sum "$OUT/ULTIHF99.EXE"
