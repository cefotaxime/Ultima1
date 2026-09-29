# M257 — Time Machine backdrop preservation, final wording, BIOS-size gems

M257 corrects three details of the first `MONDAIN.EXE` Time Machine page.

1. The final three Korean rows are:
   - `첫째는 '발사'라고 적힌 버튼이 있다는 것,`
   - `둘째는 당신이 기체 안에 갇혔기 때문에`
   - `그 버튼을 누를 수밖에 없다는 것입니다!`
   They remain centered on source BIOS rows 14, 15 and 16.

2. The DOS transition chains from `OUT.EXE` into `MONDAIN.EXE` without clearing the framebuffer. `MONDAIN.EXE` image `0x1B9B` only clears the popup rectangle through image `0x047D`. M257 therefore preserves the frozen overworld backing view and composites the M256 popup over it.

3. The four coloured marks are BIOS teletype character `0x04`, not a narrow host-font control glyph. `MONDAIN.EXE` image `0x1C1A..0x1C3D` positions row 7 at columns `12+5*i` and calls image `0x30D5` with EGA colours 12/10/9/15. M257 renders a full 8x8 source BIOS diamond, displayed as a 16x16 cell at 2x scale.

Legacy audit compatibility is retained: M97 Ready expressions remain verbatim, M255/M256 are forward-compatible, and M257 pins the framebuffer/gem path.
