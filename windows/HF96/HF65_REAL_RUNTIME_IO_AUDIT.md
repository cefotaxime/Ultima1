# HF65 real runtime differential checkpoint

AppleWin 1.32.1.0 (actual Ultima I 1986 ProDOS program disk) was driven to original overworld and Britain town. The debugger's BSAVE command captured both authentic 65,536-byte RAM states, saved in research/overworld_ram_original.bin and research/britain_ram.bin. The original OUT and TWN overlays are present in those respective snapshots and validated against extracted on-disk binaries.

Original SPA overlay was also tested from the genuine overworld snapshot. The CPU interpreter successfully reached original code at $8956 and progressed into the actual SPA startup routines. However it repeatedly invokes resident routines accessing Apple II vertical blank status at $C019, keyboard at $C000 and disk-controller soft switches; simply replaying CPU instructions without a hardware model is not sufficient to complete SPA boot. A test $C019 signal oscillating on a 17,030-cycle period escaped the initial infinite wait but did not fully finish loading original SPA data. This is a concrete outstanding technical gap, not an audio filter issue.

For 542 normal-flight $9477 calls using the genuine overworld RAM + SPA program overlay, observed 10,110,048 NMOS 6502 cycles, 8,597 speaker toggles, 0 reads of $C019, 1,626 keyboard reads and 542 reads of Disk II soft switch range $C0E0..$C0EF. Thus speaker simulation currently cannot be called identical to a running original Apple II system: disk/keyboard responses are presently static emulator RAM values.

Full HyperJump source at $9811 using genuine intro RAM had 7,629,630 cycles and 2,767 speaker toggles; no VBL accesses, but 417 keyboard and 413 disk soft-switch reads. The independent py65 reference matched every toggle cycle.

HF65 full original-source sound+visual integration was automatically exercised at all 8 front-view speeds. All eight complete stages 1..5, return to flight, and deduct exactly 100 fuel once; observed completion in seconds from command: speed1 50.55; speed2 29.233; speed3 7.9167; speed4 7.9667; speed5 8.0; speed6 8.05; speed7 8.1; speed8 8.15.

Automated comparison of first 1 second with archived original video: HF64 character-creation RAM and HF65 overworld RAM had identical band-integrated energy distribution, approximately 12.91 percentage-point L1 spectral error vs reference. Yet generated full 11-second WAV files are NOT byte-identical (305,932 differing bytes). Hence comparing only first-second spectrum insufficient to declare sonic accuracy.

No public EXE produced; CPU program not completely equivalent to original hardware until the actual space state and machine I/O model are recovered. Files remain in the dedicated work directory.