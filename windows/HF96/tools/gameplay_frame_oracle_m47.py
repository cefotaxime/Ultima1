#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw
out=Path(__file__).resolve().parents[1]/'m47_gameplay_frame_mask.png'
im=Image.new('RGB',(320,200),(0,0,0))
d=ImageDraw.Draw(im)
blue=(0,0,170)
# source-coordinate frame bands
for box in [(0,0,319,7),(0,0,7,159),(312,0,319,159),(0,152,319,159),(240,160,247,199)]:
    d.rectangle(box,fill=blue)
# Mark the certified text-cell extents only as one-pixel guides in grayscale.
# Command region: cols 0..29, rows 21..24. Status: cols 31..39, rows 21..24.
guide=(64,64,64)
d.rectangle((0,168,239,199),outline=guide)
d.rectangle((248,168,319,199),outline=guide)
im.save(out)
print(out)