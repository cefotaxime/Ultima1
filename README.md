# Ultima I Korean Localization — M257 Time Machine backdrop/gem restoration

M257 keeps the M256 framed Time Machine modal while restoring the DOS transition semantics around it: the existing overworld framebuffer remains visible behind the popup instead of the upper viewport being cleared to black. The backing view is frozen while the modal waits for Space/Return.

The final three Korean rows are updated to the approved `첫째는... / 둘째는... / 그 버튼을...` wording and remain centered. The four RGBW markers now use a full 8x8 BIOS character-4 diamond at 2x presentation (16x16 host cell), matching `MONDAIN.EXE`'s BIOS teletype path and EGA colours 12/10/9/15.

Historical Ready/audit contracts remain intact.
