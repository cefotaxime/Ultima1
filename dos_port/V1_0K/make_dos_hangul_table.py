
from pathlib import Path
import re
root=Path(__file__).resolve().parent
src=(root.parents[1]/'src'/'HF93'/'src'/'PixelFont.cpp').read_text('utf8')
names=['kChoNoJong','kChoWithJong','kJungNoJong','kJungWithJong','kJongSet']
a={}
for n in names:
 m=re.search(r'\b'+n+r'\[\d+\]\s*=\s*\{([^}]+)\}',src)
 if not m: raise ValueError(n)
 a[n]=[int(x.strip()) for x in m.group(1).split(',')]
lines=['#pragma once']
for n in names:
 v=a[n]
 lines.append('static const unsigned char '+n+'['+str(len(v))+']={'+','.join(map(str,v))+'};')
(root/'dos_hangul_table.h').write_text('\n'.join(lines)+'\n','utf8')
print('generated',[(k,len(v)) for k,v in a.items()])
