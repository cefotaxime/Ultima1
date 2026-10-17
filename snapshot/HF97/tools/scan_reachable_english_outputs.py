from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]/"src"
pat=re.compile(r'(?:pushCommand|\.push\(|push_back\(|appendLast\(|replaceLastCommand\()\s*\(?.*?"([^"\\]*(?:\\.[^"\\]*)*)"')
hits=[]
for p in root.rglob("*.cpp"):
    for i,line in enumerate(p.read_text(encoding="utf-8",errors="ignore").splitlines(),1):
        if not any(k in line for k in ["pushCommand","push_back","log.push","appendLast","replaceLastCommand"]): continue
        for lit in re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"',line):
            if re.search(r'[A-Za-z]{3,}',lit):
                hits.append((p.name,i,lit,line.strip()))
for f,i,lit,line in hits:
    print(f"{f}:{i}: {lit} || {line}")
print("TOTAL",len(hits))
