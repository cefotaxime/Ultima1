#!/usr/bin/env python3
from pathlib import Path
import json, sys
try:
    from PIL import Image
except Exception as e:
    raise SystemExit('Pillow is required to emit PNG oracle images: '+str(e))

root=Path(__file__).resolve().parents[1]
data=Path(sys.argv[1]) if len(sys.argv)>1 else root/'gamedata'
out=Path(sys.argv[2]) if len(sys.argv)>2 else root/'m44_oracle_output'
out.mkdir(parents=True,exist_ok=True)

def pick(name):
    p=data/name
    if p.exists(): return p
    p=data/name.lower()
    if p.exists(): return p
    raise SystemExit(f'missing {name} in {data}')

u=pick('ULTIMA.EXE').read_bytes()
castle=pick('CASTLE.16').read_bytes()
if len(u)!=36880: raise SystemExit('ULTIMA.EXE must be 36,880 bytes')
if len(castle)!=32000: raise SystemExit('CASTLE.16 must be 32,000 bytes')

# Exact ORIGIN mask via B26's two 64-entry DS pointer tables.
BASE=0x40D0; LP=BASE+0x3262; RP=BASE+0x4320
W,H,LW,RW=275,64,199,76
def w(off): return u[off] | (u[off+1]<<8)
logo=Image.new('RGB',(W,H),(0,0,0)); px=logo.load()
for y in range(H):
    l=BASE+w(LP+2*y); r=BASE+w(RP+2*y)
    a=u[l:l+LW]; b=u[r:r+RW]
    if len(a)!=LW or len(b)!=RW or not set(a+b)<=set(b' *'):
        raise SystemExit('bad logo row pointer')
    row=a+b
    for x,ch in enumerate(row): px[x,y]=(255,255,255) if ch==42 else (0,0,0)
logo.save(out/'m44_origin_logo_oracle.png')

# Exact CASTLE.16 decoded through the ULTIMA launcher crossover palette.
ega=[(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),
     (85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
cross=[0,8,7,15,15,2,10,6,12,7,13,11,1,9,7,15]
title=Image.new('RGB',(320,200)); tp=title.load(); pos=0
for y in range(200):
    for x in range(0,320,2):
        v=castle[pos]; pos+=1
        tp[x,y]=ega[cross[v>>4]]; tp[x+1,y]=ega[cross[v&15]]
title.save(out/'m44_title_oracle.png')

manifest={
  'source_resolution':[320,200],
  'ega':{'text_white':15,'rule_blue':1},
  'copyright':{
    'text':[{'cell':[16,8],'text':'Ultima I','color':15},
            {'cell':[8,11],'text':'The First Age of Darkness','color':15},
            {'cell':[0,21],'text':'Copyright (C) 1987 Origin Systems, Inc.','color':15}],
    'rects':[{'xyxy':[112,58,200,60],'color':1},{'xyxy':[112,74,200,76],'color':1}]
  },
  'logo':{'xyxy':[20,20,294,83],'size':[275,64]},
  'presents':{'text':[{'cell':[14,13],'text':'Presents...','color':15}]},
  'release':{'erase':[{'cell':[14,13],'text':'           '}],
             'text':[{'cell':[5,12],'text':'...a new release of the best','color':15},
                     {'cell':[5,13],'text':'   selling personal computer','color':15},
                     {'cell':[5,14],'text':'   role-playing adventure...','color':15}]},
  'masterpiece':{'text':[{'cell':[6,12],'text':"...Lord British's original",'color':15},
                         {'cell':[6,13],'text':'   fantasy masterpiece...','color':15}]},
  'credits':{'text':[{'cell':[1,17],'text':'ULTIMA and LORD BRITISH are registered','color':15},
                     {'cell':[2,18],'text':'trademarks of Richard Garriott and','color':15},
                     {'cell':[11,19],'text':'Origin Systems, Inc.','color':15},
                     {'cell':[6,23],'text':'Conversion by: John Fachini','color':15}]},
  'timing_ticks':{'presents':45,'release':75,'masterpiece':75,'title':300,'credits':60},
  'attract_order':['copyright','presents','release','masterpiece','title','presents'],
  'keypress_exit':['credits','GEN.EXE']
}
(out/'m44_launcher_geometry.json').write_text(json.dumps(manifest,indent=2)+"\n")
print(out)
