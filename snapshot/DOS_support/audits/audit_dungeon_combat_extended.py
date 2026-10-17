from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
main=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
dng=(r/"src/HF93/src/DungeonSystem.cpp").read_text(encoding="utf-8")
start=main.index('else if(k==SDLK_a){',main.index('dungeonSystem.attack(dungeon,p,dm);')-1600)
end=main.index('else if(k==SDLK_z){',start)
attack=main[start:end]
checks={
"attack_launch_sfx_queued":'dungeonPlayerSfxQueue.push_back(sfxId(SfxEvent::PlayerAttackLaunch))' in attack,
"hit_sfx_queued_after_launch":attack.index('SfxEvent::PlayerAttackLaunch')<attack.index('SfxEvent::CombatHit'),
"lethal_hit_message_preserved":'dm.resize(1);' in attack,
"lethal_tail_deferred":'dungeonLethalDeferred.assign(dm.begin()+1,dm.end());' in attack,
"predeath_sprite_rendered":'dungeonLethalPrePresent ? dungeonLethalVisualSnapshot : dungeon' in main,
"damage_frame_presented_before_kill_text":main.index('presentFrame();',main.index('// M231: OUT.EXE draws first'))<main.index('pushMessages(dungeonLethalDeferred);'),
"kill_text_requires_second_present":'dungeonLethalResultNeedsPresent=true;' in main and 'else if(dungeonLethalResultNeedsPresent)' in main,
"kill_updates_dungeon_cell":'target.monster=0xFFFF; target.hp=0;' in dng,
"kill_updates_experience":'d.exitHitBonus+=expGain*2;' in dng,
"no_target_skips_launch":'if(dungeonHasTarget) dungeonPlayerSfxQueue.push_back' in attack,
}
for k,v in checks.items():print(("PASS" if v else "FAIL")+" "+k)
assert all(checks.values())