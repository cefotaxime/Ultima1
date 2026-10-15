from pathlib import Path
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix30\GAMEDATA\OUT.EXE").read_bytes()
calls=[]
for off in range(len(b)-7):
    if b[off]==0xB8 and b[off+2]==0 and b[off+3]==0x50 and b[off+4]==0xE8:
        rel=int.from_bytes(b[off+5:off+7],'little',signed=True)
        if off+7+rel==0x0676:
            calls.append((off,b[off+1]))
from collections import Counter
print("count",len(calls),"ids",dict(sorted(Counter(n for _,n in calls).items())))
for off,n in calls:
    print(f"{off:04x} sound({n})")