from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix26\GAMEDATA\OUT.EXE").read_bytes()
for i in range(10):
    print(i+1,hex(int.from_bytes(b[0x13e5+i*2:0x13e7+i*2],"little")))
