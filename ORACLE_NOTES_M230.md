# M230 — OUT.EXE PC-speaker routine emulation

M230는 M225~M229의 per-ID WAV runtime playback을 폐기하고 원본 OUT.EXE의 sound(0)..sound(9) software port-61h routine을 source-level로 복원한다.

Generic Noise gate는 file 0x0676, dispatcher table은 DS:24CA = file 0xFB4A이며 0..9는 AE65, AE7D, AE9B, AEA2, AEA9, AEC9, AED0, AEEA, AF1E, AF37로 연결된다.

복원한 loop topology:
- sound0: CX=0x10, BX=1000
- sound1: CX=0x30, BX=0x163
- sound2/3/4: common DH+=5 loop, BL=C8/FF/08
- sound5/7: nested CH/CL/DL loop
- sound6: BH=A0 shrinking delay
- sound8: BL=FB increasing INC BH/JNZ delay
- sound9: DH 5,10,15,20

M225 direct capture calibration에서 source routine span은 0..9 각각 47.574, 50.981, 77.948, 98.631, 4.302, 899.246, 39.064, 248.070, 97.042, 0.936 ms이다. capture WAV의 더 긴 filter tail은 source return time과 분리된다.

Local movement는 OUT.EXE 0x30E9..0x3120 순서에 맞춰 새 위치를 먼저 present하고, 그 다음 synchronous sound(4)를 실행한 뒤 actor update로 넘어간다.

Host transport는 continuous short SDL queued stream을 사용한다. per-effect queue clear/requeue와 complete-WAV FIFO는 사용하지 않는다. PULSE_LATENCY_MSEC=5를 요청하고 play()는 consumed sample clock이 source end를 지날 때까지 return하지 않는다.

M225 capture WAV는 런타임 음원이 아니라 regression/listening oracle로만 보존한다.
