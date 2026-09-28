# M250 — SPACE command localization and Sector Scan reveal

## SPACE command strings
User-approved Korean command rows are now used throughout SPACE mode: 공격하기?, 타기?, 시전하기?, 버리기?, 진입하기?, 발포하기?, 가지기?, 하이퍼점프, 탐지하기, 뭐라고요?, 오르기?, 효과음 켜기/끄기, 열기?, 게임 종료하기?, 장비하기?, 훔치기?, 거래하기?, 자물쇠 열기?, 시점 바꾸기, 내리기?, 상태창 보기.

All SPACE command rows retain the normal blue > prompt metadata. The no-BEEP HyperJump refusal remains 하이퍼점프 without ?.

## Sector Scan initial reveal
SPACE.EXE file 0x175E..0x1787 proves the initial 7x7 scan draw uses x=0..6 as the OUTER loop and y=0..6 as the INNER loop, calling the cell-glyph routine for every cell. Therefore original hardware visibly draws one full column at a time from left to right. SDL buffering previously collapsed all 49 calls into one present.

M250 exposes the recovered column order one presentation quantum at a time, then starts the existing current-sector blink. The blink remains source-exact in identity/cadence: 0x17C7 and 0x180C each call delay(7), approximately 385 ms per visible/blank phase.
