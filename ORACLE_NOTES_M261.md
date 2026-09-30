# M261 — MONDAIN Attack row, avatar sound(4), and input-wait animation

M261 restores three remaining MONDAIN.EXE presentation behaviors.

- Attack stays on the live prompt-bearing BIOS row 24. The chosen direction is appended on that same row.
- The normal final-arena Attack path passes sound id 7 into the attack core before hit/ray resolution.
- Successful player movement blits the new player cell and then calls sound(4) before the command returns.
- The final-arena keyboard reader calls animation routine 0x1477 each polling cycle; with no key pending it waits 3 BIOS ticks. Evil-gem pulse, transient magic/flame slots 14..19, and Mondain sprite animation therefore advance while waiting rather than once per turn.

Historical M111 was updated so main.cpp owns the live Attack command row and MondainSystem emits only result rows; it no longer requires the obsolete duplicated Attack prompt.
