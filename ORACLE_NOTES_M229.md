# M229 — continuously-fed queued-audio mixer

M228 still called `SDL_ClearQueuedAudio()` and resubmitted the not-yet-played suffix whenever a new sound arrived. Repeated queue replacement can restart/starve the WSLg/PulseAudio path, matching the observed delayed/bunched footsteps.

M229 keeps the M225-proven SDL queued-audio device, but uses a **continuous short-horizon stream**. A dedicated SDL feeder thread wakes every 1 ms, mixes active capture voices into short PCM chunks, and maintains only a few milliseconds of queued lead. New `sound()` calls add a voice to the future source timeline; they do not clear or restart SDL's queue.

The M225 capture/filter tail remains audible and overlaps later calls naturally. For sound(4), consecutive player/NPC calls retain the 5 ms source spacing while their longer filter tails overlap. No complete-WAV FIFO and no per-call queue replacement remain.

M227's one-KEYDOWN-per-outer-frame rule remains. `logicalBusy()` models only the original synchronous source duration, never the audible tail.
