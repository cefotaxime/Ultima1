#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="${1:?Usage: bash stage_runtime.sh OUTPUT_DIRECTORY EXTERNAL_GAMEDATA_DIRECTORY}"
DATA="${2:?External GAMEDATA directory required}"
[[ -d "$DATA" ]] || { echo "Missing game data directory: $DATA" >&2; exit 2; }
for font in H12.FNT ROBO.FNT; do
 [[ -s "$ROOT/assets/FONTS/$font" ]] || { echo "Missing GitHub-tracked font: $font" >&2; exit 2; }
done
mkdir -p "$OUT/FONTS" "$OUT/GAMEDATA" "$OUT/SAVEGAME"
cp "$ROOT/assets/FONTS/H12.FNT" "$ROOT/assets/FONTS/ROBO.FNT" "$OUT/FONTS/"
cp -a "$DATA/." "$OUT/GAMEDATA/"
echo "STAGED: fonts from GitHub, external game data from $DATA; save directory not populated"
