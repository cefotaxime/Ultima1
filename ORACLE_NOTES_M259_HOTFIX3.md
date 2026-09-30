# M259 hotfix3 — relocation audit order/cache semantics

Hotfix2 still failed deterministically after a successful CMake configure/build. The launcher correctly deleted any stale `build/` tree before configure, but `audit_m259_hotfix1_relocatable_build.py` ran after the build and incorrectly required `build/CMakeCache.txt` to be absent. A successful CMake configure necessarily creates that file.

Hotfix3 keeps the early build-tree cleanup and changes the relocation audit to validate the regenerated cache instead. After configure/build, the audit now requires `CMakeCache.txt` to exist and checks that `CMAKE_HOME_DIRECTORY:INTERNAL` points to the current extracted source tree. A cache from another source path is rejected.

A new hotfix3 regression audit pins the required order: cleanup -> configure/build -> relocation-cache validation. The canonical final marker remains `[audit] All audits through M259 OK`; the package revision is emitted separately as `M259 hotfix3`.
