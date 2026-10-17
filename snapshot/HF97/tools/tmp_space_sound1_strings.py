from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix30\GAMEDATA\SPACE.EXE").read_bytes()
for off in [0xb31,0xc26,0xc2e,0xc35,0xc3b]:
    e=b.find(b'\x00',off)
    raw=b[off:e]
    print(hex(off),raw.hex(' '),repr(raw.decode('ascii','replace')))
