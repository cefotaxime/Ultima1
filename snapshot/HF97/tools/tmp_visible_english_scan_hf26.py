from pathlib import Path
import re
root=Path(r"C:\\Users\\hojun\\Downloads\\Ultima1_KR_1.0K_Source_HF25\src")
files=['main.cpp','LocalSimulation.cpp','VehicleSystem.cpp','KingSystem.cpp','ShopSystem.cpp','CommandSystem.cpp','DungeonSystem.cpp','SpaceSystem.cpp','FrontendSystem.cpp']
pat=re.compile(r'"([^"\\]*(?:\\.[^"\\]*)*)"')
for fn in files:
    for i,line in enumerate((root/fn).read_text(encoding='utf-8').splitlines(),1):
        code=line.split('//',1)[0]
        for m in pat.finditer(code):
            s=m.group(1)
            if re.search(r'[A-Za-z]{2,}',s):
                print(f'{fn}:{i}: {s}')
