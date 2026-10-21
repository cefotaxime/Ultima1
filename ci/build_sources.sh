#!/usr/bin/env bash
# Clean GitHub checkout -> Windows and DOS executables and runtime stage.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="${RUNNER_TEMP:-/tmp}/ultima1-ci"
mkdir -p "$WORK" "$ROOT/dist/stage-win" "$ROOT/dist/stage-dos"
if [[ -z "${DJGPP_CXX:-}" ]]; then
  curl -fL --retry 3 -o "$WORK/djgpp.tar.bz2" \
    'https://github.com/andrewwutw/build-djgpp/releases/download/v3.4/djgpp-linux64-gcc1220.tar.bz2'
  tar -xjf "$WORK/djgpp.tar.bz2" -C "$WORK"
  export DJGPP_CXX="$WORK/djgpp/bin/i586-pc-msdosdjgpp-g++"
fi
[[ -x "$DJGPP_CXX" ]] || { echo 'DJGPP compiler unavailable' >&2; exit 2; }
export U1_DOS_BUILD_DIR="$WORK/dos-build"
bash "$ROOT/snapshot/DOS_support/build_full_from_source.sh"
test -s "$U1_DOS_BUILD_DIR/ULTIHF99.EXE"
cp "$U1_DOS_BUILD_DIR/ULTIHF99.EXE" "$ROOT/dist/stage-dos/ULTIHF99.EXE"

if [[ -z "${SDL2_DIR:-}" ]]; then
  curl -fL --retry 3 -o "$WORK/SDL2.zip" \
    'https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-devel-2.30.0-mingw.zip'
  unzip -q "$WORK/SDL2.zip" -d "$WORK"
  export SDL2_DIR="$WORK/SDL2-2.30.0/x86_64-w64-mingw32/lib/cmake/SDL2"
fi
test -f "$SDL2_DIR/sdl2-config.cmake"
cmake -S "$ROOT/snapshot/HF97" -B "$WORK/windows-build" \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DSDL2_DIR="$SDL2_DIR"
cmake --build "$WORK/windows-build" --target u1kr_clean -j 4
cp "$WORK/windows-build/u1kr_clean.exe" "$ROOT/dist/stage-win/Ultima1KR.exe"
x86_64-w64-mingw32-strip --strip-debug "$ROOT/dist/stage-win/Ultima1KR.exe"
sdk_root="$(cd "$SDL2_DIR/../../.." && pwd)"
cp "$sdk_root/bin/SDL2.dll" "$ROOT/dist/stage-win/SDL2.dll"
for name in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
  found="$(find /usr/lib/gcc /usr/x86_64-w64-mingw32 /usr/lib -name "$name" -type f 2>/dev/null | head -n 1 || true)"
  [[ -n "$found" ]] || { echo "Missing MinGW runtime $name" >&2; exit 3; }
  cp "$found" "$ROOT/dist/stage-win/$name"
done
for dir in "$ROOT/dist/stage-win/FONTS" "$ROOT/dist/stage-dos/FONTS"; do
  mkdir -p "$dir"
  cp "$ROOT/assets/FONTS/H12.FNT" "$ROOT/assets/FONTS/ROBO.FNT" "$dir/"
done
cp "$ROOT/installer/dos/dosbox_ultima1.conf" "$ROOT/installer/dos/Ultima1K.bat" "$ROOT/dist/stage-dos/"
echo 'SOURCE_BUILDS_COMPLETE'
sha256sum "$ROOT/dist/stage-win/Ultima1KR.exe" "$ROOT/dist/stage-dos/ULTIHF99.EXE"
