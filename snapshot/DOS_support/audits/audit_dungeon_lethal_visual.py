from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
s=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
checks={
"snapshot_before_attack":s.index("const DungeonState dungeonBeforeAttack=dungeon;")<s.index("dungeonSystem.attack(dungeon,p,dm);"),
"detect_lethal":'dungeonBeforeAttack.cells[ci].monster!=0xFFFFu' in s and 'dungeon.cells[ci].monster==0xFFFFu' in s,
"preserve_predeath_sprite":'dungeonLethalVisualSnapshot=dungeonBeforeAttack;' in s,
"hold_kill_message":'dungeonLethalDeferred.assign(dm.begin()+1,dm.end());' in s,
"keep_hit_message":'dm.resize(1);' in s,
"render_preserved_frame":'dungeonLethalPrePresent ? dungeonLethalVisualSnapshot : dungeon' in s,
"release_kill_message_after_render":s.index('dungeonLethalPrePresent ? dungeonLethalVisualSnapshot : dungeon')<s.index('pushMessages(dungeonLethalDeferred);'),
}
for name,ok in checks.items():print(("PASS " if ok else "FAIL ")+name)
assert all(checks.values())