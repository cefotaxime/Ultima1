from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
m=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
k=(r/"src/HF93/src/KingSystem.cpp").read_text(encoding="utf-8")
l=(r/"src/HF93/src/LocalSimulation.cpp").read_text(encoding="utf-8")
tests={
"castle_king_entry":'king.begin(original,local,p,storeroomAllowance,m)' in m,
"king_refuses_if_not_on_throne":'localRawTile(local.mapIndex,local.x,local.y)!=0x3E' in k,
"king_refuses_when_alert":'if(local.alert)' in k,
"king_quest_reward_state_reset":'p.kingQuest[static_cast<std::size_t>(castleIndex_)]=0;' in k,
"king_service_popup_stage":'stage_=KingStage::ServicePopup;' in k,
"king_service_key_handler":'king.handleKey(ch,p,m)' in m,
"princess_adjacent_required":'if(!princessAdjacent(local)) return out;' in l,
"princess_reward_coin_hp_exp":all('player.'+x+'=static_cast<std::uint16_t>(std::min(9999' in l for x in ('coin','hits','experience')),
"extra_princess_reward_conditions":'player.spaceKills>=20u && player.experience>=7000u' in l,
"princess_popup_invoked":'princessRescuePresentation.begin();' in m,
"castle_guard_hit_sounds":'SfxEvent::LocalGuardAttack' in m and 'SfxEvent::CombatHit' in m,
"jail_unlock_wired":'simulation.unlock(local,p)' in m,
"jail_requires_key":'if(!local.foundKey)' in l,
"jail_checks_correct_key":'tile!=local.correctKeyTile' in l,
"jail_opens_door":'local.tiles[static_cast<std::size_t>(local.x*OriginalData::LOCAL_H+(local.y-1))]=1;' in l,
}
for name,ok in tests.items():print(("PASS " if ok else "FAIL ")+name)
assert all(tests.values())