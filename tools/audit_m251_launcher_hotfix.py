#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
note=(root/'ORACLE_NOTES_M251_LAUNCHER_HOTFIX.md').read_text(encoding='utf-8')
for q in ['GAMEDATA: unbound variable','AUDIT_DATA','before rm -rf']:
    assert q in note,q
print('M251 launcher hotfix documentation audit OK')
