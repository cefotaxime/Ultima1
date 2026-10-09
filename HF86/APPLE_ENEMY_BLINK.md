# HF86 Apple II front-view enemy kill blink

Based on HF85. The 1986 Apple II `SPA.bin` code at `$96BD..$96D0` alternates draw/erase of a hit enemy eight times, pausing normal flight and background updates. HF86 implements eight approximately 64-ms cycles (32-ms visible and 32-ms invisible, ~512 ms total) in `SpaceSystem::drawFightExact`, freezing simulation in `SpaceSystem::updateFrame` until completion, keeping the existing enemy position and stage, and deferring target respawn/clear until the effect ends. `main.cpp` ignores gameplay key commands during the blink. Original DOS hitbox and score calculation are preserved. Win64 MinGW build succeeded; interactive visual comparison remains pending.

Package on the remote PC: `C:\Users\hojun\Downloads\Ultima1_KR_Work\packages\HF86_APPLE_ENEMY_BLINK.zip`.
