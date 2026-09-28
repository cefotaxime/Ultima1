#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
note=(root/'ORACLE_NOTES_M252.md').read_text(encoding='utf-8')
for q in ['하이퍼점프 완료','0x3530','DS:0C9E','[0,8,7,6,5,4,3,2,1]','0x5EF7','0x2377..0x23AC']:
    assert q in note,q
print('M252 HyperJump timing subset documentation audit OK')
