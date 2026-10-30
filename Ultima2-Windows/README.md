# Ultima II (DOS CGA) — native Windows port

**Status: bootstrap only, not a playable port.**

This is a *separate* experimental work area on a dedicated branch. It targets the
**original IBM PC DOS/CGA release** of Ultima II, not the Voyager Dragon EGA upgrade.

## Principles

- Preserve original gameplay, rules, data and quirks unless explicitly approved.
- Load legally obtained original game files at runtime; **never commit game binaries,
  maps, graphics, music, player saves, or disassembled copyrighted code**.
- Do not derive implementation code by copying a proprietary binary. Any behavioral
  reimplementation will be documented and independently verified.
- Retain the original 320×200 CGA logical display; investigate composite artifact
  colors as an optional output mode. Do not confuse RGB CGA with composite CGA.
- Keep localization separate from core simulation and drawing.
- Store work in GitHub; no development artifacts are written to the user's PC.

## Current deliverable

A minimal native Win32 executable that opens a window and inspects a folder of
**user-supplied original DOS game files**. It displays the recognized asset
inventory and provides a `--probe` CLI mode. It does **not** execute the DOS game,
draw original game tiles, simulate movement, or claim to reproduce gameplay.

## Build (GitHub Actions)

The workflow `ultima2-win32-bootstrap.yml` builds using MSVC on
`windows-latest` without any original game assets. The binary is uploaded
as a temporary workflow artifact.

Local development is not required. Later, a test build may be downloaded to a
user-approved testing location when interactive verification is needed.

## CLI

```powershell
Ultima2Win.exe --probe "C:\path\to\ultima2"
Ultima2Win.exe "C:\path\to\ultima2"
```

The folder is selected at runtime; the program does not require or copy the
original files into this repository. The fixed asset lengths below describe the
single provided reference distribution and should not be treated as universal.

## Next engineering gates

1. Confirm original executable/data variants and map all file formats.
2. Analyze tile packing, map bytes, palette, and original CGA draw routines.
3. Implement a deterministic native rendering engine with reference screenshots.
4. Recreate original input, world transitions, combat, inventory, save/load,
   event loop and timing; compare against the DOS reference.
5. Only after functional parity: Korean text support and UI.

The original and EGA-upgrade archives are treated as **different reference
distributions**; no upgraded engine behavior is imported without review.
