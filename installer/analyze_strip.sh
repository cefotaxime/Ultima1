#!/bin/bash
set -euo pipefail
SRC=/mnt/c/Users/hojun/.cache/ultima1-build/legacy-installer-payload
OUT=/mnt/c/Users/hojun/.cache/ultima1-build/win-minimal-stage
mkdir -p "$OUT"
cp -a "$SRC"/. "$OUT"/
for f in "$OUT"/*.dll "$OUT"/*.exe; do
 echo "BEFORE $(basename "$f") $(stat -c%s "$f")"
 x86_64-w64-mingw32-objdump -h "$f" | grep -E 'debug|\.text|\.rdata' | head -n 12 || true
 x86_64-w64-mingw32-strip --strip-debug "$f"
 echo "AFTER  $(basename "$f") $(stat -c%s "$f")"
done
