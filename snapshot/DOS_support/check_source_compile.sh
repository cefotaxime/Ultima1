#!/usr/bin/env bash
set -euo pipefail
# Source-only DOS object compilation checkpoint. Does not link or modify game/save data.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/snapshot/DOS_support/HF93_src"
PORT="$ROOT/snapshot/DOS_support/V1_0K"
OUT="${U1_DOS_BUILD_DIR:-${TMPDIR:-/tmp}/ultima1-dos-source-check}"
CXX="${DJGPP_CXX:-$HOME/.cache/ultima1-build/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++}"
# The default is a Windows home mount; override DJGPP_CXX on other hosts.
if [[ ! -x "$CXX" && -x /mnt/c/Users/hojun/.cache/ultima1-build/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++ ]]; then
 CXX=/mnt/c/Users/hojun/.cache/ultima1-build/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++
fi
[[ -x "$CXX" ]] || { echo 'Missing DJGPP compiler; set DJGPP_CXX to its path' >&2; exit 2; }
mkdir -p "$OUT"
for name in KingSystem LocalSimulation; do
 echo "COMPILE $name"
 "$CXX" -std=gnu++17 -O1 -I"$SRC" -I"$PORT" -include "$PORT/dos_singlethread_mutex.h" -c "$SRC/$name.cpp" -o "$OUT/$name.o"
done
echo 'CHECKPOINT_OK: two DOS objects built from tracked sources without legacy object files'
