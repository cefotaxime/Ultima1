#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$HERE/build/u1kr_clean"
[[ -x "$BIN" ]] || { echo "Build first: ./build_and_run_wsl.sh ..." >&2; exit 1; }
mkdir -p "$HERE/m13_captures"
for s in straight_corridor transitions_secret environment_objects monster_blocker; do
  echo "Capturing $s"
  "$BIN" --data "$HERE/gamedata" --h12 "$HERE/fonts/H12.FNT" --robo "$HERE/fonts/ROBO.FNT" \
    --save "$HERE/gamedata/PLAYER1.U1" \
    --dungeon-scenario "$HERE/scenarios/$s.txt" \
    --capture-bmp "$HERE/m13_captures/$s.bmp" --capture-once
 done
echo "Captures are in: $HERE/m13_captures"