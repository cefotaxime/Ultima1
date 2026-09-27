# M239 — zero-idle direct PulseAudio / read-cursor prime

M238 proved that bypassing SDL alone did not change the user's approximately three-step audible onset delay. Re-reading the direct Pulse path exposed a transport mistake: while the game was idle, the PulseAudio write callback still rendered zero PCM whenever the server requested data. There was no application FIFO, but there was server-side queued silence. A newly scheduled sound therefore could start behind silence already accepted by PulseAudio.

M239 removes that condition without changing the approved PC-speaker synthesis, pitch, source timing, or game timing.

- When no voice is pending, the Pulse write callback returns without writing any PCM.
- prebuf=0, so an idle underrun does not require the stream to refill a server prebuffer before playback can resume.
- At sound() start, pulseKick() immediately writes at most 64 frames (1.333 ms at 48 kHz) using PA_SEEK_RELATIVE_ON_READ. This rebases the stale write cursor at the current Pulse read cursor after idle.
- Ordinary subsequent write callbacks append with PA_SEEK_RELATIVE; they do not repeatedly seek to the read cursor and therefore do not overwrite already queued sound.
- Once the real voice/filter tail is exhausted, the callback again stops writing instead of manufacturing idle silence.

The 64-frame prime is an output transfer quantum, not an added delay. The original synchronous sound() source duration remains the lower bound for the routine's return exactly as in M238 hotfix1.

audio_runtime_m239.txt records the selected backend, negotiated Pulse buffer values, first reported stream latency, and underflow count.
