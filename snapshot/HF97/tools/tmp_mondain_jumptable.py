from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix26\GAMEDATA\MONDAIN.EXE").read_bytes()[0x200:]
for i in range(11):
    print(i,hex(int.from_bytes(b[0xe72+i*2:0xe74+i*2],"little")))
