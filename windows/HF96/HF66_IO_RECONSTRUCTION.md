# HF66 — original 1986 Apple II peripheral read-site restoration

Primary question: Are 417 keyboard and 413 disk switch reads during actual $9811 HyperJump attributable to genuine relevant hardware responses or virtual-RAM artifacts?

Using unmodified 1986 SPA/STUPH/MI.U1 code, traced instruction PCs at every $C000/$C0E0-$C0EF access:
- PC $1D67 reads $C000 via original keyboard poll at $1D64; 417 visits.
- PC $160B follows instruction `BIT $C0E8` at $1608; 413 visits. $C0E8 is a Disk II motor-control soft switch, not a stream of disk data. Virtual RAM return value is therefore not legitimate.
- Disassembly of $1D64: LDA $C000 / BPL RTS; if a key has bit7 set, conditionally handles ASCII and clears strobe with BIT $C010. Stale $C000 byte from captured game RAM must not be interpreted as a perpetually held key.

HF66 source change: audio-only virtual $C000, $C010 and $C0E8 no-key/motor-off soft-switch reads return zero instead of stale captured RAM bytes. This removes stale-key side effects. No arbitrary audio oscillator was introduced.

Measured original full $9811 HyperJump at speed 8 under corrected peripheral model:
- 2,269,498 executed 6502 instructions
- 7,624,578 CPU cycles
- 2,767 $C030 speaker toggles
- Independent py65 vs C++ differential for **each speaker timestamp**: **0 mismatches**
Old (stale keyboard RAM) model: 2,271,595 instructions; 7,629,630 cycles; 2,767 toggles. The keyboard latch materially affected instruction and timing history.

Automated game/audio integration at all speeds 1-8 passed, including stage transitions, return, fuel 1000->900 and survival; full run approx 5.5 seconds of test wall-clock runtime.

Important limitations: This rebuild correctly handles those three observed read sites; it does not prove entire Apple II Disk II, video or keyboard behavior is emulated. Actual original in-space RAM snapshot is still missing, and local reference recording comparison is not completed at precisely synchronized audio stages. For this reason HF66 remains a source-level research prototype; do not issue a user-facing EXE/package yet.