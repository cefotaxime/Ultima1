# M238 — canonical WSL direct PulseAudio transport

The user's canonical launch path is `build_and_run_wsl.sh`. M237's Windows-only IAudioClient3 path therefore did not participate in the user's real runtime. M238 corrects that assumption: WSL/Linux is now the primary audio-transport target.

The M236 hotfix1 PC-speaker synthesis remains byte-for-byte unchanged. M238 changes only the host transport. On WSL/Linux it opens WSLg's PulseAudio server directly with libpulse's asynchronous write-callback API. The stream requests an overall 5 ms playback target through `PA_STREAM_ADJUST_LATENCY`; `minreq`, `prebuf`, and `maxlength` are left for the server to negotiate. This deliberately avoids repeating M232's unstable 2 ms / aggressive cadence experiment.

Canonical WSL path:

```
PcSpeakerEmulator 48 kHz mono S16
-> M235 source-synchronous render clock
-> libpulse asynchronous write callback
-> PA_STREAM_ADJUST_LATENCY, tlength=5 ms request
-> WSLg PulseAudio RDPSink
-> Windows host audio
```

SDL remains an automatic fallback and can be forced with `U1_SFX_FORCE_SDL=1` for A/B testing. No runtime WAV playback, `SDL_QueueAudio`, application PCM FIFO, feeder thread, or idle-silence prefill is introduced.

M238 writes the selected transport and the PulseAudio server's negotiated `tlength` / `minreq` to `audio_runtime_m238.txt` in the package directory. This makes the actual WSL path visible without needing to catch terminal output behind the game window.

The existing Windows M237 IAudioClient3 path is retained as a secondary runtime path, but it is not the canonical validation environment for this project.
