# Source and asset storage policy

Approved 2026-10-17.

- GitHub `cefotaxime/Ultima1`: localized game source, build scripts, tests, documentation, and project-created `assets/FONTS/H12.FNT` and `assets/FONTS/ROBO.FNT` (creator provenance confirmed by project owner).
- Outside GitHub: original commercial Ultima I game data, third-party fonts without established redistribution rights, user save games, personal data, prebuilt DOS/Windows binaries, compiler SDK installations, and temporary build outputs.
- Preserve a separate backup of irreplaceable save games before changing any game installation. Never overwrite user saves during automated build or runtime tests.
- Build scripts should accept asset paths as arguments or environment variables; they should not assume the old `Downloads/Ultima1_KR_Work` folder exists.
- Keep the old work directory untouched until independent Windows and DOS executable builds plus runtime checks have passed.

- Release package rule (confirmed 2026-10-17): BOTH Windows and DOS packages MUST include `H12.FNT` and `ROBO.FNT` from tracked `assets/FONTS`. Package verification fails if either is missing.
- Original commercial game data and third-party runtime components are kept outside source control; do not publicly publish bundles containing them without separate rights review.
