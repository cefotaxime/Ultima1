#!/usr/bin/env python3
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
mondain=(root/'src/MondainSystem.cpp').read_text(encoding='utf-8')
main=(root/'src/main.cpp').read_text(encoding='utf-8')
build=(root/'build_and_run_wsl.sh').read_text(encoding='utf-8')

for q in [
    'centered(14,"첫째는 \'발사\'라고 적힌 버튼이 있다는 것,");',
    'centered(15,"둘째는 당신이 기체 안에 갇혔기 때문에");',
    'centered(16,"그 버튼을 누를 수밖에 없다는 것입니다!");',
]: assert q in mondain,q

assert 'const bool mondainIntroBlocking=mondain.active && mondain.introActive;' in main
assert '} else if(mondain.active && !mondain.introActive){' in main
assert 'const bool normalOverworldWait=mondainIntroBlocking || normalOverworldWaitBase;' in main
assert 'const std::uint32_t animationNow=mondainIntroBlocking' in main
assert '? overworldAnimEpochMs+overworldAnimProcessedStep*OVERWORLD_ANIM_MS' in main
assert 'const std::uint32_t animStep=normalOverworldWait' in main
assert 'if(mondainIntroBlocking) mondainSystem.draw(r,font,mondain);' in main

assert 'static constexpr std::uint8_t BIOS_GEM_04[8]={0x00,0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10};' in mondain
assert 'SDL_Rect q{cellX+xx*2,cellY+yy*2,2,2};' in mondain
assert 'for(int i=0;i<4;++i) drawBiosGem(COLOR_COLS[i]*16,7*16,gemColors[i]);' in mondain

assert 'if(!space.active) drawReadyPopup(r,font,commands,p);' in main
assert 'if(commands.active()) drawReadyCommandInput(r,font,original,commands);' in main

m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=257
m=re.search(r'Starting clean-room M(\d+) \(canonical WSL runtime\)',build); assert m and int(m.group(1))>=257

if len(sys.argv)>1:
    data=Path(sys.argv[1]); mon=next((p for p in [data/'MONDAIN.EXE',data/'mondain.exe'] if p.exists()),None)
    assert mon,'MONDAIN.EXE not found'; b=mon.read_bytes()
    assert hashlib.sha256(b).hexdigest()=='5533c4658f73476170ac63d792c72fab6178826b866e39c296862cae01244ced'
    img=b[0x200:]
    assert bytes.fromhex('bb 05 00 f7 eb 05 0c 00 bb 07 00 53 50 e8') in img[0x1c12:0x1c2d]
    assert bytes.fromhex('b8 04 00 50 e8') in img[0x1c33:0x1c40]
    assert bytes.fromhex('b4 0e 8a 46 04 8a 5e 06 cd 10') in img[0x30d5:0x30ef]

print('M257 Time Machine backdrop + 16x16 BIOS gem audit OK')
