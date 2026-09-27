# M241 — rollback to verified M238 hotfix1 audio

M240 failed user validation: sound character changed and audible onset regressed from about 3 steps to about 6 steps.

M241 restores src/SfxSystem.cpp and src/SfxSystem.h from the verified M238 hotfix1 baseline. PcSpeakerEmulator remains unchanged. M239/M240 read-clock/reanchor logic is explicitly rejected as a baseline.

Future latency work must preserve this rollback state first and change one transport variable at a time.
