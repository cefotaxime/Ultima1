# M256 — Time Machine popup geometry + centered Korean text

M256 corrects M255's presentation of the first `MONDAIN.EXE` Time Machine page.

The page does not own the whole 320x200 screen. `MONDAIN.EXE` image `0x1B9B` calls image routine `0x047D`, which clears source rectangle `(22,15)..(300,143)`. Image routine `0x0423` then draws the modal outline through `(25,18) -> (297,18) -> (297,140) -> (25,140) -> (25,18)`. The normal Ultima outer frame and lower command/status panes remain visible.

The approved nine-row Korean translation is preserved, with prose rows centered inside the modal. RGBW source anchors stay at BIOS columns 12/17/22/27; `붉은색` is positioned so `은` starts at the original R column. The four coloured gem marks remain on row 7.

The Space/Return prompt is rendered in the normal lower command pane, and the post-page Mondain handoff rows are deferred until the modal is acknowledged.

Historical audit compatibility was updated: M97 Ready predicates remain verbatim inside an outer modal guard, M255 is now forward-compatible, and M256 pins the recovered popup geometry and presentation.
