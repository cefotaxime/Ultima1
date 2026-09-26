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

# Exact enemy-attack routine oracle.
assert out[0x83FC:0x83FC+15]==bytes.fromhex(
    '55 83 ec 16 8b ec 33 c0 c7 46 14 01 00 b1 04'
)

attack=out[0x83FC:0x8678]

# Path blit uses physical tile 50 (0x32), then:
# first source step -> sound(3); later steps -> delay(2).
assert bytes.fromhex('bb 32 00 53') in attack
assert bytes.fromhex('83 7e 14 01 75 0b b8 03 00 50') in attack
assert bytes.fromhex('b8 02 00 50') in attack

# Successful hit performs another tile-50 blit on the player cell (9,4).
assert bytes.fromhex('b8 04 00 50 b8 09 00 50 b8 32 00 50') in attack

# Simulation must retain every drawn world cell BEFORE mountain/player stop checks.
for q in [
    'struct WorldEnemyAttackEvent',
    'std::vector<std::pair<int,int>> path;',
    'bool hit=false;',
    'int damage=0;',
    'event.path.push_back({x,y});',
    'const int terrain=original_.terrainAt(x,y);',
    'if(terrain==3) break;',
    'if(x==static_cast<int>(p.x) && y==static_cast<int>(p.y))',
    'event.damage=damage;',
    'enemyAttackEvents_.push_back(std::move(event));',
    'std::vector<WorldEnemyAttackEvent> OverworldSimulation::takeEnemyAttackEvents()',
]:
    assert q in hdr or q in sim,q

# Presentation order:
# prefix first -> tile50 path -> result suffix on same logical row.
for q in [
    'log.push(enemyAttackVisual.attackPrefix);',
    'log.appendLast(enemyAttackVisual.resultSuffix);',
    'SDL_Texture* effect=original.physicalTexture(50);',
    'enemyAttackVisualIndex<enemyAttackVisual.path.size()',
]:
    assert q in main,q

# Enemy projectile playback blocks keyboard input and B157 wait-animation RNG.
assert 'enemyAttackVisualActive' in main and 'localActorDialoguePresentation.active()) continue;' in main
assert '!enemyAttackVisualActive &&' in main

# First source blit has no delay(2). M212 restores the actual synchronous
# sound(3) duration when Noise is enabled; later cells retain delay(2).
for q in [
    'else if(enemyAttackVisualIndex==0)',
    'sfx.durationMs(SfxEvent::EnemyRangedLaunch)',
    'advance=enemyAttackVisualFramePresented && soundDone;',
    'SDL_GetTicks()-enemyAttackVisualStartedMs)>=WORLD_PROJECTILE_STEP_MS',
]:
    assert q in main,q

# Successful hit gets the second player-cell flash plus sound(2) before result finalization.
for q in [
    'if(enemyAttackVisual.hit){',
    'enemyAttackHitFlash=true;',
    'if(noiseEnabled) sfx.playImmediate(SfxEvent::CombatHit);',
    'sfx.durationMs(SfxEvent::CombatHit)',
    'if(enemyAttackHitFlash)',
]:
    assert q in main,q

# HP is logically projected during the simulation pass, then restored for
# presentation and applied only after the projectile/hit flash completes.
for q in [
    'const std::uint16_t hitsBeforeEnemyPass=p.hits;',
    'p.hits=hitsBeforeEnemyPass;',
    'if(enemyAttackVisual.hit && enemyAttackVisual.damage>0)',
    'damage>=static_cast<int>(p.hits)',
]:
    assert q in main,q

# Focused executable test pins the visible route, including the terminal mountain cell.
for q in [
    'const std::vector<std::pair<int,int>> expected={{16,8},{17,8},{18,8}};',
    'std::pair<int,int>{129,10}',
    'mountain terminal cell was not retained in visual path',
    'could not exercise both hit and miss',
]:
    assert q in test,q

assert 'add_executable(u1kr_enemy_projectile_test_m118' in cmake
assert 'SDL_CreateWindow("Ultima I KR clean-room M' in main

print('M118 OUT.EXE 0x83FC enemy projectile presentation audit OK')
