# M227 — source-timed overlapping SFX

M225 capture-oracle PCM itself is retained. The remaining lag came from host-side serialization, not from the approved waveform.

M225 appended complete capture WAVs through SDL queued audio. That is wrong for the DOS PC speaker because the captured waveform contains an impulse/filter tail that continues after the original synchronous sound() routine has returned. For sound(4), the logical source call lasts about 5 ms while the captured audible tail is roughly 39 ms.

M227 therefore uses a small SDL audio callback mixer. Consecutive original sound() calls are source-spaced by logicalDurationMs(), while their longer capture tails overlap. No full-WAV FIFO and no stale-tail clearing remain.

The main loop also dispatches at most one KEYDOWN per outer frame. While a source-level sound sequence is logically busy, gameplay keys remain in SDL's event queue. The busy interval is based only on the original source duration, never the audible filter tail.

For local movement this means player sound(4) begins first, then each successful local actor movement contributes another sound(4) about 5 ms later. Their filter tails overlap instead of forming a 39+39+39 ms train.
