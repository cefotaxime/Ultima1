#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
readme=(root/'README.md').read_text(encoding='utf-8')
note=(root/'ORACLE_NOTES_M250.md').read_text(encoding='utf-8')
for q in ['공격하기?','게임 종료하기?','자물쇠 열기?','x=0..6 as the OUTER loop','left to right']:
    assert q in (readme+'\n'+note),q
print('M250 SPACE translation / Sector Scan subset documentation audit OK')
