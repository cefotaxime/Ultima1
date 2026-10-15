from pathlib import Path
import re
root=Path(r"C:\\Users\\hojun\\Downloads\\Ultima1_KR_1.0K_Source_HF27\src")
for p in sorted(root.glob('*.cpp')):
  for i,line in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
    code=line.split('//',1)[0]
    if not any(k in code for k in ['font.draw','drawText','popupLines_','popupMessage_','title=','label=']):
      continue
    lits=re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"',code)
    eng=[s for s in lits if re.search(r'[A-Za-z]{2,}',s)]
    if eng: print(f'{p.name}:{i}: {eng} :: {code.strip()}')