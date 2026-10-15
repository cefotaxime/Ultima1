from pathlib import Path
import re, json
root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF41\src")
rows=[]
for p in root.rglob("*.cpp"):
    s=p.read_text(encoding="utf-8",errors="ignore")
    for i,line in enumerate(s.splitlines(),1):
        if '"' not in line: continue
        lits=re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"',line)
        for lit in lits:
            # English-looking UI candidates; skip obvious code/paths/symbolic tokens.
            if not re.search(r'[A-Za-z]{3,}',lit): continue
            if re.fullmatch(r'[A-Za-z0-9_./:+\- ]+',lit) is None and not any(c.isalpha() for c in lit): continue
            rows.append((str(p.relative_to(root)),i,lit,line.strip()))
out=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF41\tools\english_literal_candidates.txt")
out.write_text("\n".join(f"{f}:{i}: {lit} || {line}" for f,i,lit,line in rows),encoding="utf-8")
print("candidates",len(rows))
for r in rows[:250]:
    print(f"{r[0]}:{r[1]}: {r[2]}")