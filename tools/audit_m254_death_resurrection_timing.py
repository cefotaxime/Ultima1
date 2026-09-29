#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
note=(root/'ORACLE_NOTES_M254.md').read_text(encoding='utf-8')
for q in ['0xC67C','sound(2) three times','delay(0x50)','readyArmour<4','0x37DF']:
    assert q in note,q
print('M254 death/resurrection documentation audit OK')
