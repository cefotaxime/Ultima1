# Ultima I Korean Localization — M252 HyperJump timing restoration

M252 removes the final period from `하이퍼점프 완료` and restores the source speed-dependent acceleration/deceleration pacing recovered from SPACE.EXE.

- HyperJump phase 1 accelerates from the original speed through speed 8.
- Each phase-1 update calls source routine 0x3530, which performs the front-view update and then waits by the DS:0C9E speed table: speeds 0..8 => retrace counts [0,8,7,6,5,4,3,2,1].
- The retrace helper at 0x5EF7 counts EGA/VGA vertical retraces; mode 0Dh is 70 Hz.
- HyperJump phase 4 decelerates from speed 8 toward the original speed and explicitly applies the same retrace wait after each update.
- The 201-call and 200-call middle starfield phases remain unchanged.

The validated M246 WASAPI/PCM transport remains unchanged.
