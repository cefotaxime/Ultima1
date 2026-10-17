from pathlib import Path
p=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix43\GAMEDATA\SPACE.EXE")
b=p.read_bytes()
for start,end in [(0x24B7,0x2600)]:
    print(f"RANGE {start:04X}-{end:04X}")
    for off in range(start,end):
        op=b[off]
        if op==0xE8 and off+2<end:
            rel=int.from_bytes(b[off+1:off+3],"little",signed=True)
            print(f"CALL {off:04X} -> {off+3+rel:04X}")
        elif op in (0xE9,0xEB):
            if op==0xE9 and off+2<end:
                rel=int.from_bytes(b[off+1:off+3],"little",signed=True); print(f"JMP  {off:04X} -> {off+3+rel:04X}")
            elif op==0xEB and off+1<end:
                rel=int.from_bytes(b[off+1:off+2],"little",signed=True); print(f"JMP8 {off:04X} -> {off+2+rel:04X}")
