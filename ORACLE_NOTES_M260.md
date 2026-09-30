# M260 — Mondain combat localization and final-battle presentation

M260 applies the approved Korean combat strings for the Mondain encounter and restores the original presentation split between the command pane and the framed modal.

## Defeat
`MONDAIN.EXE` raw 0x14F2 first emits `THOU ART DEAD!` through the normal command-output routine. It then calls the same framed popup initializer used by the Time Machine narration and writes the defeat body at source rows 6/7 and 12. M260 follows that structure: the lower command pane receives `당신은 사망하였습니다!`; the upper framed popup contains `당신은 마술사 몬데인에게 / 패배하였습니다!` and `우주의 운명은 끝입니다!`.

## Victory
`MONDAIN.EXE` raw 0x20D6 calls the framed popup initializer, then prints `THOU ART VICTORIOUS!` at source row 8. M260 therefore leaves the final combat messages in the lower command pane and overlays `승리하였습니다!` in the framed upper popup. The NIF.BIN ending is a separate subsequent phase rather than appearing on the lethal-hit frame itself.

## Combat text
The approved translations for chanting, Mondain's three spells, misses, stat loss, melee attack, player/Mondain damage, false death, gem burn/destruction, Get Gem, Inform/Search, and the two Inform results are applied directly to the Mondain command stream.

## Time Machine page 2
The first paragraph is now four rows (`불빛이 춤을 추기` / `시작합니다.`), preserving exactly one empty row between the three paragraphs.
