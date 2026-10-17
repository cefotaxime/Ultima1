# Source and asset storage policy

Approved 2026-10-17.

- GitHub `cefotaxime/Ultima1`: localized game source, build scripts, tests, documentation, and legally distributable source dependencies.
- Outside GitHub: original commercial Ultima I game data, proprietary fonts where redistribution rights have not been established, user save games, personal data, prebuilt DOS/Windows binaries, compiler SDK installations, and temporary build outputs.
- Preserve a separate backup of irreplaceable save games before changing any game installation. Never overwrite user saves during automated build or runtime tests.
- Build scripts should accept asset paths as arguments or environment variables; they should not assume the old `Downloads/Ultima1_KR_Work` folder exists.
- Keep the old work directory untouched until independent Windows and DOS executable builds plus runtime checks have passed.
