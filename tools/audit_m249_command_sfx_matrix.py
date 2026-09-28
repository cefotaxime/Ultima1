#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
readme=(root/'README.md').read_text(encoding='utf-8')
note=(root/'ORACLE_NOTES_M249.md').read_text(encoding='utf-8')
for q in ['H now BEEPs outside SPACE','Q BEEPs outside the field','Valid dungeon Attack','Dungeon Klimb/Open/Unlock']:
    assert q in (readme+'\n'+note),q
print('M249 command-SFX subset documentation audit OK')
