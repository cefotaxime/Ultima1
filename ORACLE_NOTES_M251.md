# M251 — dungeon combat SFX completion

OUT.EXE byte audit:

- 0xE61: dungeon player valid Attack calls sound(7).
- 0x1108 and 0x1187: successful dungeon player hits call sound(2), for surviving and lethal targets respectively.
- 0x8A06: dungeon monster attack attempts call sound(3).
- 0x8BBE: normal damaging monster hits additionally call sound(2).
- Misses and non-damage special effects skip the follow-up sound(2).

M246 PCM/WASAPI transport is unchanged.
