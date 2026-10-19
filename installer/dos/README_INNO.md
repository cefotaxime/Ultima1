# DOS installer (Inno Setup 6)

The standard Inno Setup edition is an alternative to the original custom C# installer. It avoids the custom managed executable containing a ZIP resource. The build does **not** recompile or modify the Korean DOS game executable; it bundles the already verified binary, DOSBox, fonts, and launcher configuration.

## Build

Run on Windows with Inno Setup 6.7.3 installed:

```powershell
powershell.exe -NoProfile -File installer\dos\build_inno_dos.ps1 -StageDir "C:\path\to\dos-installer-stage" -OutputDir "C:\path\to\output"
```

The stage must contain ULTIHF99.EXE, CWSDPMI.EXE, DOSBox\DOSBox.exe, DOSBox SDL DLLs, COPYING.txt, AUTHORS.txt, FONTS\H12.FNT and ROBO.FNT, dosbox_ultima1.conf, and Ultima1K.bat. The build script verifies staged fonts against the repository hashes. Third-party tools and original commercial game assets are **not** checked in. DOSBox license obligations and permission to redistribute its dependencies must be reviewed before public distribution.

## Installation behavior

The installer asks for a separately licensed original DOS Ultima I directory, requires ULTIMA.EXE and eight other data/executable files, and copies the required original game assets into a **separate** GAMEDATA directory within the Korean installation. It does not patch the original EXE. SAVEGAME is created but not overwritten by the installer; existing saves are preserved. The included DOSBox runs ULTIHF99.EXE using the staged configuration.

## Local validation (2026-10-19)

- Inno compiler 6.7.3 built the trial installer successfully.
- Silent isolated installation exited with status 0; game executable, DOSBox and both project fonts had the same SHA-256 hashes as staged files; original GAMEDATA files were copied; a pre-existing SAVEGAME sentinel remained unchanged.
- Bundled DOSBox launched and remained running for six seconds. In-game DOS behavior was not visually validated in this test.
- Local Microsoft Defender scan completed without a new detection. **This does not establish VirusTotal or cloud reputation clearance.** Previous custom C# installer was reported detected by several VirusTotal engines including Microsoft; an independent scan of the new file is still needed. No false-positive claim is made before that review.

Trial executable is intentionally **not** committed to GitHub. Public release also depends on applicable third-party licensing and game-data distribution rights.
