# M260 hotfix1 — M164 historical audit compatibility

M260 intentionally translated the isolated Mondain encounter's `Missed!` as the approved `빗맞음!`. Historical `audit_m164_miss_noise_pub_seduction.py` incorrectly forced both `LocalSimulation.cpp` and `MondainSystem.cpp` to contain the generic local-combat wording `공격이 빗나갑니다!`, so it rejected the intended M260 localization.

Hotfix1 narrows that historical audit back to its subsystem contract:
- LocalSimulation keeps `공격이 빗나갑니다!`.
- MondainSystem requires `빗맞음!` and must not expose English `Missed!`.
- Dungeon remains `빗맞음!`.

A dedicated M260 hotfix1 compatibility audit pins this regression. No gameplay code is changed.
