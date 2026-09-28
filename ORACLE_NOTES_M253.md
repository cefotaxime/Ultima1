# M253 — HyperJump blocking command-row ownership

SPACE.EXE keeps HyperJump inside one blocking command routine. The command row therefore remains on BIOS row24 for the whole animation and no new input cursor is created until the routine returns to the command loop.

Visible states:
- >하이퍼점프...
- >하이퍼점프...광속 추진!
- >하이퍼점프 완료

Only after completion does the row become retained history and the normal row24 command cursor reappear.

M252 pacing and M246 audio transport are unchanged.
