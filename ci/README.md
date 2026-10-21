# GitHub-only build and distribution

The **source of truth** is the public `cefotaxime/Ultima1` repository. No path under a developer's `Downloads`, `AppData`, WSL home, or local backup is needed by the CI workflow.

## Automatic build

[Build Windows and DOS installers from GitHub](../.github/workflows/release-build.yml) runs on pushes affecting the game/installer and can be started manually with **Actions → Build Windows and DOS installers from GitHub → Run workflow**.

1. A clean Ubuntu runner downloads versioned DJGPP GCC 12.2.0 (build-djgpp v3.4) and SDL2 MinGW SDK 2.30.0, installs MinGW-w64, and compiles both game executables from checked-in source.
2. A fresh Windows runner receives only the built outputs, downloads DOSBox and CWSDPMI, and builds the native Windows installer and the Inno Setup DOS installer.
3. Both EXEs are individually compressed as `Ultima1K(Win)_Setup.7z` and `Ultima1K(DOS)_Setup.7z`. The workflow verifies both archives and publishes SHA-256 checksums.
4. Download the **Ultima1K-Windows-DOS-Installers-7z** artifact from the successful Actions run. Versioned GitHub Releases can retain the two 7z files permanently.

## Source boundaries

- Tracked: Korean game source, DOS port, both installer sources, scripts, test code, custom H12.FNT/ROBO.FNT fonts, and DOS launcher configuration.
- Downloaded by CI: compilers, SDL2, DOSBox, CWSDPMI, Inno Setup and 7-Zip. These are independently available third-party dependencies.
- **Not** included: copyrighted original Ultima I game files, personal save games, private directories or credentials. The installers ask the user to select their own licensed original DOS Ultima I installation.
- This builds the current source revision, not necessarily a byte-for-byte identical copy of a previously approved binary. Compiler/toolchain changes and installer timestamps can change hashes. Treat a new release as a new candidate until runtime and antivirus testing is complete.

## Acceptance criteria

A GitHub-only build is accepted only when **both** jobs are green, the artifact includes both 7z files, `7z t` passes, and the SHA-256 manifest matches. GUI gameplay validation and external malware-reputation scans are separate release gates. If any dependency download or compilation fails, the workflow must fail instead of silently falling back to local files.

No original game assets or saves are needed to compile the binaries; licensed original game files are needed only for installation and gameplay.
