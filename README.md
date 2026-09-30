# Ultima I Korean Localization — M259 hotfix3

M259 fixes Mondain arena slot mapping and the Time Machine intro. Hotfix3 fixes the relocation audit itself.

The launcher removes any old `build/` tree before CMake configure. After configure/build, `CMakeCache.txt` is expected to exist; the relocation audit now verifies that the cache belongs to the current extracted source tree via `CMAKE_HOME_DIRECTORY` instead of incorrectly requiring the cache to be absent.

The canonical final marker remains `[audit] All audits through M259 OK`, with `M259 hotfix3` reported separately.
