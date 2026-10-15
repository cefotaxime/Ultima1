from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix43\GAMEDATA\SPACE.EXE").read_bytes()
for target in [0x5D4C,0x65AE,0x23FC,0x634E]:
    xs=[]
    for off in range(len(b)-3):
        if b[off]==0xE8:
            rel=int.from_bytes(b[off+1:off+3],"little",signed=True)
            if off+3+rel==target: xs.append(off)
    print(hex(target), [f"{x:04X}" for x in xs])