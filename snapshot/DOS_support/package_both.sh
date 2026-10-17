#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [[ $# -lt 4 || $# -gt 6 ]]; then
 echo 'Usage: bash package_both.sh WINDOWS_EXE DOS_EXE PRIVATE_GAMEDATA_DIR OUTPUT_DIR [WINDOWS_DLL_DIRECTORY] [DOS_CWSDPMI_EXE]' >&2
 exit 2
fi
WIN="$1"; DOS="$2"; DATA="$3"; OUT="$4"; DLLS="${5:-}"; CWSDPMI="${6:-}"
for file in "$WIN" "$DOS" "$ROOT/assets/FONTS/H12.FNT" "$ROOT/assets/FONTS/ROBO.FNT"; do
 [[ -s "$file" ]] || { echo "Missing required package file: $file" >&2; exit 2; }
done
[[ -d "$DATA" ]] || { echo "Missing private GAMEDATA directory: $DATA" >&2; exit 2; }
if [[ -n "$DLLS" && ! -d "$DLLS" ]]; then echo "Missing Windows DLL directory: $DLLS" >&2; exit 2; fi
if [[ -n "$CWSDPMI" && ! -s "$CWSDPMI" ]]; then echo "Missing DOS DPMI runtime: $CWSDPMI" >&2; exit 2; fi
for platform in windows dos; do
 bash "$ROOT/snapshot/DOS_support/stage_runtime.sh" "$OUT/$platform" "$DATA"
done
cp "$WIN" "$OUT/windows/u1kr_clean.exe"
cp "$DOS" "$OUT/dos/ULTIHF99.EXE"
if [[ -n "$CWSDPMI" ]]; then cp "$CWSDPMI" "$OUT/dos/CWSDPMI.EXE"; fi
if [[ -n "$DLLS" ]]; then
 for name in SDL2.dll libgcc_s_seh-1.dll libstdc++-6.dll; do
  [[ -s "$DLLS/$name" ]] || { echo "Missing required Windows runtime DLL: $name" >&2; exit 2; }
  cp "$DLLS/$name" "$OUT/windows/$name"
 done
fi
for platform in windows dos; do
 for name in H12.FNT ROBO.FNT; do
  cmp -s "$ROOT/assets/FONTS/$name" "$OUT/$platform/FONTS/$name" || { echo "Font verification failed: $platform/$name" >&2; exit 1; }
 done
done
echo 'PACKAGE_OK: project fonts included in both Windows/DOS; saves not copied'
