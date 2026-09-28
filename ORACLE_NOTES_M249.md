# M249 — environment command BEEP matrix completion

M249 applies the user-confirmed environment-specific command sound rules that remained after M248.

H: sound(1) in field, castle/town, dungeon; SPACE HyperJump remains no-BEEP.
Q: field save/quit remains normal; castle/town, dungeon and SPACE refusals use sound(1).
Castle/town S: sound(1) when not on a storeroom marker.
Castle/town U: sound(1) when no prison-door marker is present.
Dungeon A: valid weapon uses normal attack launch sound(7); range-zero weapon keeps sound(1).
Dungeon K/O/U: missing ladder/coffin/chest uses sound(1). Chest trap keeps sound(2).

M246 WASAPI transport and approved PCM clips are unchanged.
