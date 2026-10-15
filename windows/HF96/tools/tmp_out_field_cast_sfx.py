from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix26\GAMEDATA\OUT.EXE").read_bytes()
for off in range(0x19b0,0x1d80):
    if b[off]==0xE8 and off+3<=len(b):
        rel=int.from_bytes(b[off+1:off+3],"little",signed=True)
        if off+3+rel==0x0676:
            pre=b[max(0,off-10):off]
            print(hex(off),pre.hex(" "))