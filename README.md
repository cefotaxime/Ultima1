# Ultima I Korean Localization — M249 command-SFX matrix completion

M249 preserves the user-approved M246 WASAPI/PCM transport and applies the remaining confirmed environment-specific command sound rules.

- H now BEEPs outside SPACE: field, castle/town, dungeon.
- Q BEEPs outside the field: castle/town, dungeon, SPACE. Field remains the only save/quit context.
- Castle/town Steal BEEPs when not on a storeroom marker.
- Castle/town Unlock BEEPs when no prison-door marker is present.
- Valid dungeon Attack now emits the normal attack launch sound(7); range-zero weapons retain sound(1).
- Dungeon Klimb/Open/Unlock BEEP when ladder/coffin/chest is absent.
- Dungeon chest trap remains the separate trap sound(2).

The full runnable source/package remains the canonical ZIP artifact; this GitHub repository is still the audio-focused subset.
