# Ultima I Korean Localization — M259 hotfix2

M259 fixes Mondain arena slot mapping and Time Machine intro sequencing. Hotfix2 fixes a launcher regression introduced by hotfix1: the final audit marker again uses the canonical numeric form `[audit] All audits through M259 OK`, while the package revision is printed on a separate line. This keeps the historical M258 startup audit compatible.

The distributable contains no CMake build tree or cache, so it can be unpacked and built from any WSL path.
