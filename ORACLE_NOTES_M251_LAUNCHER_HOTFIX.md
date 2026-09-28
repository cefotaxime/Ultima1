# M251 launcher hotfix — AUDIT_DATA lifetime

The M251 launcher appended the dungeon combat oracle audit after the temporary audit data directory had already been deleted and passed an undefined legacy variable named GAMEDATA. Under set -u this terminated the launcher with "GAMEDATA: unbound variable".

Hotfix:
- run audit_m251_dungeon_combat_sfx.py with "$AUDIT_DATA"
- run it before rm -rf "$AUDIT_DATA"
- add audit_m251_launcher_auditdata_lifetime.py to reject future undefined GAMEDATA references or post-cleanup oracle calls

Gameplay and audio logic are unchanged.
