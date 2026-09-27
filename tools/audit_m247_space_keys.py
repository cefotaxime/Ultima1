#!/usr/bin/env python3
# Repository-subset audit note. The full runnable package contains the executable
# source audit in tools/audit_m247_space_keys.py.
from pathlib import Path
root=Path(__file__).resolve().parents[1]
readme=(root/'README.md').read_text(encoding='utf-8')
note=(root/'ORACLE_NOTES_M247.md').read_text(encoding='utf-8')
for q in ['N now toggles','Z now opens','flight timing is frozen','M246 audio']:
    assert q in (readme+'\n'+note),q
print('M247 SPACE key subset documentation audit OK')
