#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, sys
from pathlib import Path
from PIL import Image

ROI=(16,8,304,152)  # x=16..303, y=8..151: DOS 1st-person raster only

def normalize(im: Image.Image, crop=None) -> Image.Image:
    im=im.convert('RGB')
    if crop:
        im=im.crop(tuple(crop))
    w,h=im.size
    if (w,h)==(320,200): return im
    if w%320==0 and h%200==0 and w//320==h//200:
        return im.resize((320,200),Image.Resampling.NEAREST)
    raise ValueError(f'Image is {w}x{h}; expected 320x200 or an integer scale. Use --*-crop x y w h.')

def parse_crop(v):
    if not v:return None
    x,y,w,h=map(int,v.split(',')); return (x,y,x+w,y+h)

def mask(im: Image.Image, threshold:int):
    p=im.load(); w,h=im.size
    return {(x,y) for y in range(h) for x in range(w)
            if (lambda c: (c[0]+c[1]+c[2])//3 >= threshold)(p[x,y])}

def shifted(s,dx,dy,w,h):
    return {(x+dx,y+dy) for x,y in s if 0<=x+dx<w and 0<=y+dy<h}

def metrics(ref,cand):
    inter=len(ref&cand); fp=len(cand-ref); fn=len(ref-cand)
    precision=inter/(inter+fp) if inter+fp else 1.0
    recall=inter/(inter+fn) if inter+fn else 1.0
    f1=2*precision*recall/(precision+recall) if precision+recall else 0.0
    return dict(reference_pixels=len(ref),candidate_pixels=len(cand),match_pixels=inter,
                false_positive=fp,false_negative=fn,xor_pixels=fp+fn,
                precision=precision,recall=recall,f1=f1)

def main():
    ap=argparse.ArgumentParser(description='Compare DOS Ultima I dungeon raster against clean-room capture.')
    ap.add_argument('dos'); ap.add_argument('clean')
    ap.add_argument('--dos-crop',help='x,y,w,h before normalization')
    ap.add_argument('--clean-crop',help='x,y,w,h before normalization')
    ap.add_argument('--threshold',type=int,default=180)
    ap.add_argument('--align',type=int,default=3,help='search +/- N logical pixels')
    ap.add_argument('--out-dir',default='m13_diff')
    a=ap.parse_args()
    dos=normalize(Image.open(a.dos),parse_crop(a.dos_crop)).crop(ROI)
    clean=normalize(Image.open(a.clean),parse_crop(a.clean_crop)).crop(ROI)
    rm=mask(dos,a.threshold); cm=mask(clean,a.threshold)
    w,h=dos.size
    best=None
    for dy in range(-a.align,a.align+1):
      for dx in range(-a.align,a.align+1):
        sc=shifted(cm,dx,dy,w,h); m=metrics(rm,sc)
        key=(m['f1'],-m['xor_pixels'],-abs(dx)-abs(dy))
        if best is None or key>best[0]: best=(key,dx,dy,sc,m)
    _,dx,dy,cm2,m=best
    m.update(alignment_dx=dx,alignment_dy=dy,threshold=a.threshold,roi=list(ROI))
    out=Path(a.out_dir);out.mkdir(parents=True,exist_ok=True)
    # normalized/cropped sources
    dos.save(out/'dos_roi.png'); clean.save(out/'clean_roi.png')
    diff=Image.new('RGB',(w,h),(0,0,0)); dp=diff.load()
    for q in rm&cm2: dp[q]=(255,255,255)       # exact overlap
    for q in rm-cm2: dp[q]=(255,0,0)           # missing from clean
    for q in cm2-rm: dp[q]=(0,255,255)          # extra in clean
    diff.resize((w*3,h*3),Image.Resampling.NEAREST).save(out/'diff_3x.png')
    (out/'metrics.json').write_text(json.dumps(m,indent=2)+'\n')
    print(json.dumps(m,indent=2))
    print('white=match, red=DOS-only, cyan=clean-only')
    return 0
if __name__=='__main__':raise SystemExit(main())
