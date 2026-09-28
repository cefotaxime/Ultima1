#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
note=(root/'ORACLE_NOTES_M253.md').read_text(encoding='utf-8')
for q in ['BIOS row24','no new input cursor','하이퍼점프...광속 추진!','하이퍼점프 완료']:
    assert q in note,q
print('M253 HyperJump blocking-row documentation audit OK')
