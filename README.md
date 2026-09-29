# Ultima I Korean Localization — M256 Time Machine popup restoration

M256 corrects the first Time Machine presentation in `MONDAIN.EXE`.

- Restore the original framed modal instead of clearing the entire screen.
- Exact source geometry: black clear `(22,15)..(300,143)`, blue outline `(25,18)..(297,140)`.
- Keep the normal Ultima outer frame, command pane and status pane visible.
- Center the approved Korean prose inside the popup.
- Preserve source RGBW anchors at BIOS columns 12/17/22/27; `은` in `붉은색` begins at the original `R` column.
- Put the Space/Return wait prompt in the normal lower command pane.
- Delay the post-page Mondain handoff text until the modal is dismissed.

M254 death/resurrection timing, M253 HyperJump blocking-row behavior, M252 HyperJump timing and M246 audio transport are unchanged.
