# HF89 — right alignment of status numeric values

Based on HF88. Updated `src/main.cpp` `drawStatus` and `src/SpaceSystem.cpp` space HUD to calculate a shared numeric right edge (`valueX+32`) and render digits at `rightX - font.measure(digits)`, instead of relying on left-padded strings. This anchors the final digit of one- to four-digit life/food/experience/coin values to the same x coordinate. Original HUD label positions and stat values are retained. Windows cross-compile succeeded; in-game visual verification pending.

Remote package: `C:\Users\hojun\Downloads\Ultima1_KR_Work\packages\HF89_STATUS_RIGHT_ALIGN.zip`.
