from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix26\GAMEDATA\OUT.EXE").read_bytes()
for off in range(0xb00,0x1550):
    if off+7<=len(b) and b[off]==0xB8 and b[off+2]==0 and b[off+3]==0x50 and b[off+4]==0xE8:
        rel=int.from_bytes(b[off+5:off+7],'little',signed=True)
        if off+7+rel==0x0676:
            print(hex(off), b[off+1])
