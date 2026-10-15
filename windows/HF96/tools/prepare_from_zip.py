#!/usr/bin/env python3
from pathlib import Path
import sys, zipfile

if len(sys.argv)!=3:
    print('usage: prepare_from_zip.py ULTIMA1.ZIP OUTPUT_DIR',file=sys.stderr); raise SystemExit(2)
zp=Path(sys.argv[1]); out=Path(sys.argv[2]); out.mkdir(parents=True,exist_ok=True)
# Original program/data files are mandatory, but PLAYER*.U1 saves are not.
# A completely empty roster is a valid clean starting state for the Korean port.
required={'MAP.BIN':13104,'EGATILES.BIN':6656,'EGATOWN.BIN':1632,'TCD.BIN':6840,'OUT.EXE':69858,'CASTLE.16':32000,'GEN.EXE':20112,'ULTIMA.EXE':36880}
with zipfile.ZipFile(zp) as z:
    files={Path(i.filename).name.upper():i for i in z.infolist() if not i.is_dir()}
    for name,size in required.items():
        if name not in files:
            raise SystemExit(f'missing {name} in {zp}')
        data=z.read(files[name])
        if len(data)!=size: raise SystemExit(f'{name}: expected {size} bytes, got {len(data)}')
        (out/name).write_bytes(data)
    # Keep other original files available for the next clean-room milestones.
    for name,info in files.items():
        if name in required: continue
        (out/name).write_bytes(z.read(info))
print(f'prepared original data in {out}')