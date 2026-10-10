# HF91 — Apple II 1986 star collision threshold

Based on HF90. Apple II original SPA `$8B67-$8B85` calls `$9CE2` on object indices 9 and 8 (ship and star) and compares its integer distance return to `$15` (21), branching to ship-melts death message. `$9CE2-$9D86` calculates squared coordinate differences and integer square root by successive subtraction of odd numbers. HF91 replaces the older DOS-frontend `abs(dx)<25 && abs(dy)<25` square hit zone with integer-distance threshold `<21` for the sector's star, retaining melt message and `killPlayerInSpace`. The implementation approximates the original 6502 routine for normal coordinate ranges; 8-bit saturation and exact per-object coordinate origins remain to be confirmed by interactive Apple/DOS testing. No special-case for the initial sector. MinGW Win64 build succeeded; actual gameplay death test pending.

Remote package `C:\Users\hojun\Downloads\Ultima1_KR_Work\packages\HF91_APPLE_STAR_COLLISION.zip`.
