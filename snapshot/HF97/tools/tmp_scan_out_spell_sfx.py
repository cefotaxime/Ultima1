from pathlib import Path
p=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix26\GAMEDATA\OUT.EXE")
b=p.read_bytes()
calls=[]
for off in range(len(b)-7):
    if b[off]==0xB8 and b[off+2]==0 and b[off+3]==0x50 and b[off+4]==0xE8:
        rel=int.from_bytes(b[off+5:off+7],'little',signed=True)
        if off+7+rel==0x0676:
            calls.append((off,b[off+1]))
for lo,hi,name in [(0x1500,0x1a50,'cast area'),(0x7c00,0x8500,'world-ish')]:
    print(name)
    for x in calls:
        if lo<=x[0]<hi: print(hex(x[0]),x[1])
