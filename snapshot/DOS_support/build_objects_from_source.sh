#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/snapshot/DOS_support/HF93_src"
PORT="$ROOT/snapshot/DOS_support/V1_0K"
AUDIO="$ROOT/snapshot/DOS_support/audio"
OUT="${U1_DOS_BUILD_DIR:-${TMPDIR:-/tmp}/ultima1-dos-source-check}"
CXX="${DJGPP_CXX:-/mnt/c/Users/hojun/.cache/ultima1-build/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++}"
[[ -x "$CXX" ]] || { echo 'Missing DJGPP compiler; set DJGPP_CXX' >&2; exit 2; }
mkdir -p "$OUT"
flags=(-std=gnu++17 -O1 -ffunction-sections -fdata-sections -I"$SRC" -I"$PORT" -include "$PORT/dos_singlethread_mutex.h")
compile() {
  local file="$1" name="$2"
  if [[ -f "$OUT/$name.o" && "$OUT/$name.o" -nt "$file" ]]; then echo "CACHED $name"; return; fi
  echo "COMPILE $name"
  "$CXX" "${flags[@]}" -march=i386 -c "$file" -o "$OUT/$name.o" >"$OUT/$name.log" 2>&1 || { tail -35 "$OUT/$name.log"; echo "COMPILATION_FAILED:$name"; exit 1; }
}
for n in KingSystem LocalSimulation CommandSystem ShopSystem VehicleSystem ResurrectionSystem PixelFont PcSpeakerEmulator SfxPcmM212 SfxPcmOracleM225 SfxSystem Apple1986Live MondainSystem; do
  compile "$SRC/$n.cpp" "$n"
done
for n in dos_atomic64_singlethread dos_hf93_keyboard dos_hf93_copyex dos_hf93_present dos_hf93_render_target dos_hf93_surface dos_hf93_window; do
  compile "$PORT/$n.cpp" "$n"
done
for n in dos_hf93_audio_diagnostic dos_hf93_platform dos_sb16_dma; do
  compile "$AUDIO/$n.cpp" "$n"
done
echo "COMPILE fake6502 as C"
CC="${CXX%g++}gcc"
"$CC" -std=gnu11 -O1 -c "$ROOT/snapshot/HF97/third_party/fake6502/fake6502.c" -o "$OUT/fake6502.o"
echo 'OBJECT_BUILD_OK: source-backed DOS modules built without legacy object files'
