#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv)!=2:
    raise SystemExit('usage: audit_m118_enemy_projectile.py GAMEDATA')

root=Path(__file__).resolve().parents[1]
data=Path(sys.argv[1])
out=(data/'OUT.EXE').read_bytes()
main=(root/'src'/'main.cpp').read_text(encoding='utf-8')
sim=(root/'src'/'OverworldSimulation.cpp').read_text(encoding='utf-8')
hdr=(root/'src'/'OverworldSimulation.h').read_text(encoding='utf-8')
test=(root/'tools'/'enemy_projectile_test_m118.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')

assert len(out)==69858
assert out[0x83FC:0x83FC+15]==bytes.fromhex('55 83 ec 16 8b ec 33 c0 c7 46 14 01 00 b1 04')
attack=out[0x83FC:0x8678]
assert bytes.fromhex('bb 32 00 53') in attack
assert bytes.fromhex('83 7e 14 01 75 0b b8 03 00 50') in attack
assert bytes.fromhex('b8 02 00 50') in attack
assert bytes.fromhex('b8 04 00 50 b8 09 00 50 b8 32 00 50') in attack

for q in [
    'struct WorldEnemyAttackEvent','std::vector<std::pair<int,int>> path;','bool hit=false;',
    'int damage=0;','event.path.push_back({x,y});','const int terrain=original_.terrainAt(x,y);',
    'if(terrain==3) break;','if(x==static_cast<int>(p.x) && y==static_cast<int>(p.y))',
    'event.damage=damage;','enemyAttackEvents_.push_back(std::move(event));',
    'std::vector<WorldEnemyAttackEvent> OverworldSimulation::takeEnemyAttackEvents()',
]:
    assert q in hdr or q in sim,q

for q in [
    'log.push(enemyAttackVisual.attackPrefix);','log.appendLast(enemyAttackVisual.resultSuffix);',
    'SDL_Texture* effect=original.physicalTexture(50);',
    'enemyAttackVisualIndex<enemyAttackVisual.path.size()',
]:
    assert q in main,q

assert 'enemyAttackVisualActive' in main and 'localActorDialoguePresentation.active()) continue;' in main
assert '!enemyAttackVisualActive &&' in main

for q in [
    'else if(enemyAttackVisualIndex==0)','advance=enemyAttackVisualFramePresented;',
    'SDL_GetTicks()-enemyAttackVisualStartedMs)>=WORLD_PROJECTILE_STEP_MS',
    'else if(enemyAttackVisualIndex==0) sfx.play(SfxEvent::EnemyRangedLaunch);',
]:
    assert q in main,q

# M230+ forward-compatible: locate the common present immediately before
# the post-present enemy sound block, not a milestone-specific comment.
enemy_block=main.index('if(enemyAttackVisualActive && !enemyAttackVisualFramePresented){')
present=main.rindex('presentFrame();',0,enemy_block)
enemy_sound=main.index('else if(enemyAttackVisualIndex==0) sfx.play(SfxEvent::EnemyRangedLaunch);',enemy_block)
assert present < enemy_sound

for q in [
    'if(enemyAttackVisual.hit){','enemyAttackHitFlash=true;',
    'if(enemyAttackHitFlash) sfx.play(SfxEvent::CombatHit);','if(enemyAttackHitFlash)',
    'if(enemyAttackVisualFramePresented) finishEnemyAttackVisual();',
]:
    assert q in main,q
assert 'sfx.logicalDurationMs(SfxEvent::EnemyRangedLaunch)' not in main
assert 'sfx.logicalDurationMs(SfxEvent::CombatHit)' not in main

for q in [
    'const std::uint16_t hitsBeforeEnemyPass=p.hits;','p.hits=hitsBeforeEnemyPass;',
    'if(enemyAttackVisual.hit && enemyAttackVisual.damage>0)','damage>=static_cast<int>(p.hits)',
]:
    assert q in main,q

for q in [
    'const std::vector<std::pair<int,int>> expected={{16,8},{17,8},{18,8}};',
    'std::pair<int,int>{129,10}','mountain terminal cell was not retained in visual path',
    'could not exercise both hit and miss',
]:
    assert q in test,q

assert 'add_executable(u1kr_enemy_projectile_test_m118' in cmake
assert 'SDL_CreateWindow("Ultima I KR clean-room M' in main
print('M118 OUT.EXE 0x83FC enemy projectile presentation audit OK')
