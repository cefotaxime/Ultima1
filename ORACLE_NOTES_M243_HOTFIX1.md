# M243 hotfix1 — legacy README audit contract

The M243 runtime/audio code is unchanged.

The packaged README_KO had been rewritten to a descriptive heading, while the legacy M226 audit intentionally parses the historical form:

`Ultima I 한글화 clean-room — M<milestone>`

Hotfix1 restores that header as M243. The package was then validated by running all 230 Python audit commands registered in build_and_run_wsl.sh (excluding prepare_from_zip.py); all passed.
