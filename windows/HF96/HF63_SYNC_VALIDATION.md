# HF63 original Apple II HyperJump audio / visual sync — 2026-10-09
Original 1986 SPA program source PC: $9811. C++ 6502 audio thread exposes atomic progress for $9824, $9868, $9886, $988D, $989B.
SDL visual update uses those exact 6502 caller stage/count snapshots instead of independent DOS hyper retrace timer when audio enabled.
SpaceSystem::updateAppleHyperProgress updates star modes, stars, source speed and existing fuel/sector completion.
New-H-key reset and previous-completion stage5 guard are installed.
C++ test_live_progress: all five transitions 1..5; two successive jumps both passed.
C++ test_audio_visual_sync: stage transitions at approx 0.0333, 0.2167, 3.2333, 8.0167, 8.1500 seconds; game hyperPhase returns 0 and fuel decreases from 1000 to 900 exactly once.
Build_hf63_win64: CMake/MinGW Windows executable compiled and linked successfully.
No user-facing release ZIP: input Apple II boot RAM is still simulated, and fidelity of audible waveform remains unverified; internal test binary only.