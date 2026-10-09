# HF87 Apple II-inspired Fire laser geometry

HF86->HF87 `src/SpaceSystem.cpp` changes the two laser starting points from DOS flight-window corners `(20,140)` and `(300,140)` to inset points `(40,127)` and `(280,127)` directed to the same gunsight. These approximate proportional remapping of Apple II 1986 SPA `$97BE..$97D5` original starts `(32,127)` and `(224,127)` across its 256-wide coordinate system to DOS 320-wide display. No changes to hit detection, sound, hit blink, or overall flight behavior. These are candidate visual conversion coordinates, not pixel-perfect verified equivalents. Win64 build succeeded, in-game visual validation pending.

Remote package: `C:\Users\hojun\Downloads\Ultima1_KR_Work\packages\HF87_APPLE_FIRE_LASER.zip`.
