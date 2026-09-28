#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
note=(root/'ORACLE_NOTES_M251.md').read_text(encoding='utf-8')
for q in ['0xE61','0x1108','0x1187','0x8A06','0x8BBE','sound(7)','sound(3)','sound(2)']:
    assert q in note,q
print('M251 dungeon combat SFX subset documentation audit OK')
