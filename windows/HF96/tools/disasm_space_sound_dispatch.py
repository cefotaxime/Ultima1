from pathlib import Path
from capstone import *
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix43\GAMEDATA\SPACE.EXE").read_bytes()
md=Cs(CS_ARCH_X86,CS_MODE_16)
for ins in md.disasm(b[0x0611:0x0730],0x0611):
    print(f"{ins.address:04X}: {ins.mnemonic:7} {ins.op_str}")