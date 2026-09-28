# M252 — HyperJump source timing restoration

User-visible completion row is now `>하이퍼점프 완료` with no final period.

SPACE.EXE HyperJump timing:
- 0x2271 stores the original speed.
- original speed < 4 selects 10, otherwise 15; inclusive loops yield 11 or 16 updates per speed stage.
- 0x22A2..0x22C5 accelerates through speed 8 via 0x3530.
- 0x3530 calls 0x1E2A then indexes DS:0C9E and calls 0x5EF7.
- DS:0C9E is [0,8,7,6,5,4,3,2,1] for speeds 0..8.
- 0x5EF7 waits on port 03DAh vertical retraces.
- 0x2377..0x23AC decelerates from speed 8 and applies the same retrace wait after each update.

M252 paces phase 1/4 by those original retrace waits rather than by one SDL frame per source update.
