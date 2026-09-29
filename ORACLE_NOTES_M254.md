# M254 — death/resurrection source timing restoration

OUT.EXE raw 0xC67C is the common resurrection routine reached when Hits<=0 or Food<=0.

Source order:
1. zero Coin/Food/Hits and redraw status
2. draw the DS:1A14 64x95 skull immediately
3. call sound(2) three times synchronously
4. print "<name>, thou art dead"
5. delay(0x50) BIOS ticks (~4.39 s)
6. print "Attempting resurrection!"
7. immediately set Hits to 1, then raise Hits to 99 at delay(1) per increment
8. clean overworld monsters, then raise Food to 99 with the same cadence

Confirmed SPACE fatal returns include startup Shuttle shield=0, star contact, non-Shuttle world landing, base docking with readyArmour<4 vacuum exposure, and shield depletion from Crunch/enemy fire. The readyArmour<4 check is in the base-docking service at SPACE.EXE raw 0x37DF, not the launch countdown.

M253 HyperJump blocking-row behavior, M252 HyperJump timing, and M246 audio transport remain unchanged.
