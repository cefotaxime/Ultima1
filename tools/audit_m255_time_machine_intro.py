#!/usr/bin/env python3
from pathlib import Path
import hashlib,sys,re

root=Path(__file__).resolve().parents[1]
mondain=(root/'src/MondainSystem.cpp').read_text(encoding='utf-8')
hdr=(root/'src/MondainSystem.h').read_text(encoding='utf-8')
main=(root/'src/main.cpp').read_text(encoding='utf-8')
build=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')

# Historical M255 contract. Later milestones may revise rows 14..16 and replace
# the host-font control glyph with a direct BIOS 0x04 renderer while preserving
# the same nine-row source structure.
for q in [
    '"기체에 들어선 당신은"',
    '"표시된 네 개의 홈을 발견합니다."',
    'const char* labels[4]={"붉은색","녹색","푸른색","흰색"};',
    '"알맞은 보석을 각 홈에 끼워 넣습니다."',
    '"좀 더 살펴본 당신은"',
    '"두 가지 사실을 알게 됩니다."',
]:
    assert q in mondain,q
for row in (14,15,16):
    assert f'centered({row},' in mondain

assert 'constexpr int COLOR_COLS[4]={12,17,22,27};' in mondain
assert ("font.draw(r,COLOR_COLS[i]*16,7*16,std::string(1,'\\x04'),gemColors[i]);" in mondain or
        'drawBiosGem(COLOR_COLS[i]*16,7*16,gemColors[i]);' in mondain)
assert 'bool introActive=false;' in hdr
assert 's=MondainState{}; s.active=true; s.introActive=true;' in mondain
assert 'if(s.introActive){' in mondain
assert 'if(mondain.introActive){' in main
assert 'if(k==SDLK_SPACE || k==SDLK_RETURN)' in main
assert 'mondainSystem.acknowledgeIntro(mondain,mm);' in main
assert 'const bool mondainIntroBlocking=mondain.active && mondain.introActive;' in main
assert 'drawFrames(r);' in main
assert 'if(!space.active) drawReadyPopup(r,font,commands,p);' in main
assert 'drawLog(r,font,original,log,modalInput,' in main

enter=mondain[mondain.index('void MondainSystem::enter'):mondain.index('bool MondainSystem::move')]
for old in [
    'out.push_back("Entering the craft, thou dost")',
    'out.push_back("remark upon four holes marked:")',
    'out.push_back("R    G    B    W")',
    'out.push_back("The proper gems fit in each hole.")',
]:
    assert old not in enter,old

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=255
m=re.search(r'Starting clean-room M(\d+) \(canonical WSL runtime\)',build); assert m and int(m.group(1))>=255

if len(sys.argv)>1:
    data=Path(sys.argv[1])
    mon=next((p for p in [data/'MONDAIN.EXE',data/'mondain.exe',data/'U1'/'MONDAIN.EXE',data/'U1'/'mondain.exe'] if p.exists()),None)
    assert mon,'MONDAIN.EXE not found'
    b=mon.read_bytes()
    assert len(b)==31650
    assert hashlib.sha256(b).hexdigest()=='5533c4658f73476170ac63d792c72fab6178826b866e39c296862cae01244ced'
    base=0x5B70
    for off,text in [
        (0x61a,b'Entering the craft, thou dost'),(0x638,b'remark upon four holes marked:'),
        (0x657,b'R    G    B    W'),(0x668,b'The proper gems fit in each hole.'),
        (0x68a,b'Further examination leads thee'),(0x6a9,b'to two discoveries:'),
        (0x6bd,b'There is a button marked LAUNCH,'),(0x6de,b'and thou art locked in, with'),
        (0x6fb,b'nothing to do but press it!')]:
        assert b[base+off:base+off+len(text)]==text,(hex(off),text)
    assert bytes.fromhex('3d 0d 00 74 05 3d 20 00') in b[0x3CE2:0x3D20]

print('M255 Time Machine Korean intro compatibility audit OK')
