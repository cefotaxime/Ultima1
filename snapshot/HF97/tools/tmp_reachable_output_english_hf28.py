from pathlib import Path
import re
root=Path(r"C:\\Users\\hojun\\Downloads\\Ultima1_KR_1.0K_Source_HF27\src")
for p in sorted(root.glob('*.cpp')):
    lines=p.read_text(encoding='utf-8').splitlines()
    for i,line in enumerate(lines,1):
        code=line.split('//',1)[0]
        if not re.search(r'[A-Za-z]{2,}',code): continue
        if not any(k in code for k in ['push_back(','.push(','.pushCommand(','font.draw(','return "','m.push_back(','messages.push_back(']):
            continue
        lits=re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"',code)
        eng=[s for s in lits if re.search(r'[A-Za-z]{2,}',s)]
        if eng:
            print(f'{p.name}:{i}: {eng} :: {code.strip()}')
