from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
m=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
d=(r/"src/HF93/src/DungeonSystem.cpp").read_text(encoding="utf-8")
checks={
"ladder_command_wired":"dungeonSystem.climb(dungeon,p,dm);" in m,
"ladder_requires_feature_6_or_7":"if(f!=6 && f!=7)" in d,
"upper_ladder_decrements_floor":"if(f==7){\n        --d.level;" in d,
"exit_at_floor_zero":"d.active=false; return true;" in d,
"exit_bonus_health_capped":"std::min(room,d.exitHitBonus)" in d,
"lower_ladder_increments_floor":"++d.level;" in d,
"floor_transition_regenerates":"generate(d,p);" in d,
"coffin_command_wired":"dungeonSystem.openCoffin(dungeon,p,dm);" in m,
"coffin_spawn_or_reward":"if(roll<104 && spawnMonsterAt(d,tx,ty)) return;" in d,
"chest_command_wired":"dungeonSystem.unlockChest(dungeon,p,dm);" in m,
"chest_trap_considered":"if(roll>static_cast<int>(p.agility) && p.klass!=3)" in d,
"prayer_does_not_consume_spell":"if(sp==0){" in m and "dungeonSystem.castDungeon(dungeon,p,sp,dm);" in m,
"spell_inventory_decrement":"--p.spells[static_cast<std::size_t>(sp)];" in m,
"spell_sfx_success_fail":"dungeonSystem.lastCastFailed()" in m and "SfxEvent::PositiveEffect" in m,
}
for key,ok in checks.items():print(("PASS " if ok else "FAIL ")+key)
assert all(checks.values())