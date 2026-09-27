#!/usr/bin/env python3
from pathlib import Path
import re,sys,struct

if len(sys.argv)!=2:
    raise SystemExit('usage: audit_m226_invalid_attack_weapon.py GAMEDATA')

root=Path(__file__).resolve().parents[1]
data=Path(sys.argv[1])
out=(data/'OUT.EXE').read_bytes()
assert len(out)==69858
DS=0xD680

def w(off): return struct.unpack_from('<H',out,off)[0]
def cstr(ds):
    p=DS+ds; e=out.index(0,p)
    return out[p:e].decode('cp437')

main=(root/'src'/'main.cpp').read_text(encoding='utf-8')
ora=(root/'src'/'SfxOracle.h').read_text(encoding='utf-8')
kor=(root/'src'/'KoreanText.h').read_text(encoding='utf-8')
orig=(root/'src'/'OriginalData.cpp').read_text(encoding='utf-8')
notes=(root/'ORACLE_NOTES_M226.md').read_text(encoding='utf-8')
build=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
readme=(root/'README_KO.md').read_text(encoding='utf-8')

ranges=[w(DS+0x26DE+i*2) for i in range(16)]
assert ranges == [1,1,1,1,0,1,1,3,0,0,0,1,3,1,3,3]
zero=[i for i,r in enumerate(ranges) if r==0]
assert zero == [4,8,9,10]

names=[]
for i in range(16):
    ptr=w(DS+0x2804+i*2); names.append(cstr(ptr))
assert [names[i] for i in zero] == ['rope','amulet','wand','staff']
assert cstr(0x0228)=='Attack with '
assert cstr(0x0235)=='?'

branch=bytes.fromhex(
    '83 bc de 26 00 75 14 b8 35 02 50 e8 0c 9b 8b e5 '
    'b8 01 00 50 e8 6f f3 8b e5 eb 25'
)
assert out[0x12F0:0x12F0+len(branch)] == branch
assert 0x1307 + struct.unpack_from('<h',out,0x1305)[0] == 0x0676

assert 'InvalidAttackWeapon      = 1' in ora
for text in ['"밧줄과 못"','"수호부"','"지팡이"','"마법봉"']:
    assert text in kor,text
assert 'static constexpr int range[16]={1,1,1,1,0,1,1,3,0,0,0,1,3,1,3,3};' in orig

world=main[main.index('case SDLK_a: {', main.index('switch(k)')):]
assert 'if(wr<=0){' in world
assert 'log.pushCommand(phrase+"?");' in world
assert 'beginCommandSfxHold(SfxEvent::InvalidAttackWeapon,1);' in world
assert 'pendingAttackLine=phrase+":  ";' in world

dmark='// OUT.EXE 0x12C2 checks the readied weapon range before the'
dpos=main.index(dmark)
ablock=main[dpos:dpos+1700]
assert 'if(OriginalData::weaponRange(weapon)<=0){' in ablock
assert 'log.pushCommand(phrase+"?");' in ablock
assert 'beginCommandSfxHold(SfxEvent::InvalidAttackWeapon,1);' in ablock
assert ablock.index('if(OriginalData::weaponRange(weapon)<=0){') < ablock.index('dungeonSystem.attack(dungeon,p,dm);')

assert 'if(continuation==1) finishCommand();' in main
assert 'sfx.logicalDurationMs(commandSfxHoldId)' in main

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=226
m=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cmake); assert m and int(m.group(1))>=226
assert 'audit_m226_invalid_attack_weapon.py' in build
assert re.search(r'Starting clean-room M(\\d+)',build) and int(re.search(r'Starting clean-room M(\\d+)',build).group(1))>=226
m=re.search(r'Ultima I 한글화 clean-room — M(\\d+)',readme); assert m and int(m.group(1))>=226
for q in ['Rope & Spikes','Amulet','Wand','Staff','sound(1)','0x12F0']:
    assert q in notes,q

print('M226 zero-range Ready weapon Attack ? + sound(1) audit OK')
