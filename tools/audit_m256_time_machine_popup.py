#!/usr/bin/env python3
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
mondain=(root/'src/MondainSystem.cpp').read_text(encoding='utf-8')
hdr=(root/'src/MondainSystem.h').read_text(encoding='utf-8')
main=(root/'src/main.cpp').read_text(encoding='utf-8')
build=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')

for q in [
    'sourceRect(22,15,279,129,{0,0,0,255});',
    'sourceRect(25,18,273,1,EGA_BLUE);',
    'sourceRect(297,18,1,123,EGA_BLUE);',
    'sourceRect(25,140,273,1,EGA_BLUE);',
    'sourceRect(25,18,1,123,EGA_BLUE);',
    'constexpr int POPUP_CENTER_X=(25+297);',
    'font.draw(r,POPUP_CENTER_X-font.measure(text)/2,row*16,text,WHITE);',
]: assert q in mondain,q

for q in [
    'centered(3,"기체에 들어선 당신은");',
    'centered(4,"표시된 네 개의 홈을 발견합니다.");',
    'centered(9,"알맞은 보석을 각 홈에 끼워 넣습니다.");',
    'centered(11,"좀 더 살펴본 당신은");',
    'centered(12,"두 가지 사실을 알게 됩니다.");',
]: assert q in mondain,q
for row in (14,15,16): assert f'centered({row},' in mondain

assert 'constexpr int COLOR_COLS[4]={12,17,22,27};' in mondain
assert 'font.draw(r,COLOR_COLS[0]*16-font.measure("붉"),6*16,labels[0],WHITE);' in mondain
assert ('drawBiosGem(COLOR_COLS[i]*16,7*16,gemColors[i]);' in mondain or
        "font.draw(r,COLOR_COLS[i]*16,7*16,std::string(1,'\\x04'),gemColors[i]);" in mondain)

assert 'drawFrames(r);' in main
assert 'if(!mondainIntroBlocking){' in main
assert 'mondainIntroBlocking ? std::string("스페이스바를 눌러 계속하세요: ")' in main
assert 'drawLog(r,font,original,log,modalInput,' in main
assert 'if(!space.active) drawStatus(r,font,p);' in main
assert 'if(!space.active) drawReadyPopup(r,font,commands,p);' in main
assert 'if(commands.active()) drawReadyCommandInput(r,font,original,commands);' in main

enter=mondain[mondain.index('void MondainSystem::enter'):mondain.index('bool MondainSystem::move')]
assert '...A moment later,' not in enter
assert 'void acknowledgeIntro(MondainState& s,std::vector<std::string>& out) const' in hdr
assert 'mondainSystem.acknowledgeIntro(mondain,mm);' in main

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=256
m=re.search(r'Starting clean-room M(\d+) \(canonical WSL runtime\)',build); assert m and int(m.group(1))>=256

if len(sys.argv)>1:
    data=Path(sys.argv[1]); mon=next((p for p in [data/'MONDAIN.EXE',data/'mondain.exe'] if p.exists()),None)
    assert mon,'MONDAIN.EXE not found'; b=mon.read_bytes()
    assert hashlib.sha256(b).hexdigest()=='5533c4658f73476170ac63d792c72fab6178826b866e39c296862cae01244ced'
    img=b[0x200:]
    assert bytes.fromhex('33 c0 50 b8 8f 00 50 b8 2c 01 50 b8 0f 00 50 b8 16 00 50') in img[0x047d:0x04a2]

print('M256 Time Machine framed-popup compatibility audit OK')
