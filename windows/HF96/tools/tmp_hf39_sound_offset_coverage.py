from pathlib import Path
import re
root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF39")
b=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix38\GAMEDATA\OUT.EXE").read_bytes()
calls=[]
for off in range(len(b)-7):
    if b[off]==0xB8 and b[off+2]==0 and b[off+3]==0x50 and b[off+4]==0xE8:
        rel=int.from_bytes(b[off+5:off+7],"little",signed=True)
        if off+7+rel==0x0676:
            calls.append((off,b[off+1]))
texts=[]
for p in list((root/"src").rglob("*"))+list((root/"tools").rglob("*.py")):
    if p.is_file():
        try: texts.append((p,str(p.read_text(encoding="utf-8",errors="ignore")).upper()))
        except: pass
missing=[]
for off,n in calls:
    pats=[f"0X{off:04X}",f"{off:04X}:",f"{off:04X} SOUND"]
    found=[str(p.relative_to(root)) for p,t in texts if any(x in t for x in pats)]
    if not found: missing.append((off,n))
    print(f"{off:04X}:{n} mentions={len(found)}")
print("MISSING", " ".join(f"{o:04X}:{n}" for o,n in missing))