#!/usr/bin/env python3
"""M14 exact-RGB validator for the original 320x200 dungeon raster."""
from __future__ import annotations
import argparse, json
from pathlib import Path
from PIL import Image

ROI=(16,8,304,152)  # DOS logical x=16..303, y=8..151

def logical_roi(path: str) -> Image.Image:
    im=Image.open(path).convert('RGB')
    w,h=im.size
    if (w,h)==(320,200):
        return im.crop(ROI)
    if w%320==0 and h%200==0 and w//320==h//200:
        scale=w//320
        x0,y0,x1,y1=ROI
        r=im.crop((x0*scale,y0*scale,x1*scale,y1*scale))
        # Only the DOS first-person raster is specified as replicated logical
        # pixels. The surrounding clean-room UI is intentionally higher-res.
        px=r.load(); rw,rh=r.size
        for y in range(y1-y0):
            for x in range(x1-x0):
                c=px[x*scale,y*scale]
                for yy in range(y*scale,(y+1)*scale):
                    for xx in range(x*scale,(x+1)*scale):
                        if px[xx,yy]!=c:
                            raise ValueError(f'{path}: non-replicated scaling in dungeon ROI at logical ({x+x0},{y+y0})')
        return r.resize((x1-x0,y1-y0),Image.Resampling.NEAREST)
    raise ValueError(f'{path}: expected 320x200 or an integer-scale 320x200 presentation')

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('dos')
    ap.add_argument('clean')
    ap.add_argument('--out',default=None)
    a=ap.parse_args()
    d=logical_roi(a.dos)
    c=logical_roi(a.clean)
    dp=list(d.getdata()); cp=list(c.getdata())
    bad=[i for i,(x,y) in enumerate(zip(dp,cp)) if x!=y]
    result={'roi':list(ROI),'pixels':len(dp),'rgb_mismatch':len(bad),'pass':not bad}
    if bad:
        xs=[i%d.width for i in bad]; ys=[i//d.width for i in bad]
        result['bbox_xyxy_roi']=[min(xs),min(ys),max(xs)+1,max(ys)+1]
    if a.out:
        Path(a.out).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return 0 if not bad else 1

if __name__=='__main__': raise SystemExit(main())