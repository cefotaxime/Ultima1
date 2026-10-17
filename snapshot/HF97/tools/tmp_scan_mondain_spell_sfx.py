from pathlib import Path
p=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix25\GAMEDATA\MONDAIN.EXE")
img=p.read_bytes()[0x200:]
calls=[]
for off in range(len(img)-7):
    if img[off]==0xB8 and img[off+2]==0 and img[off+3]==0x50 and img[off+4]==0xE8:
        rel=int.from_bytes(img[off+5:off+7],'little',signed=True)
        if off+7+rel==0x053d:
            calls.append((off,img[off+1]))
for off,n in calls:
    if 0x0a00 <= off <= 0x0e50:
        print(hex(off), n)
