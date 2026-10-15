from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix30\GAMEDATA\SPACE.EXE").read_bytes()
for target in (0x110e,0x1bbd):
    print("target",hex(target))
    for off in range(len(b)-3):
        if b[off]==0xE8:
            rel=int.from_bytes(b[off+1:off+3],'little',signed=True)
            if off+3+rel==target:
                print(hex(off), b[max(0,off-16):off+5].hex(' '))