# M236 — native Windows SDL2/WASAPI comparison path

M235 user runtime result: DOSBox-style impulse sound quality remained correct, but local footstep audible onset was still about six steps late. M235 had already removed the application PCM queue, feeder thread, `SDL_QueueAudio`, and idle-silence prefill, and `play()` waited for the SDL callback sample clock to cross the original source span. Therefore M236 does **not** retune audio timing or speaker synthesis again.

M236 adds a second execution path using the exact same game/SFX source code but native Windows SDL2 with the WASAPI backend. This removes WSLg/PulseAudio from the comparison path:

- WSL path: game → SDL2 → PulseAudio/WSLg → Windows audio engine → device
- native path: game → SDL2/WASAPI → Windows audio engine → device

`build_and_run_windows.ps1` uses CMake + vcpkg SDL2, forces `SDL_AUDIODRIVER=wasapi` for the run, prepares the same original Ultima data/fonts, and launches the same `u1kr_clean` target. `-Trace` enables the existing `U1_SFX_TRACE=1` diagnostics.

No runtime WAV playback has been reintroduced. M234/M235 impulse synthesis and M235 callback-clock synchronization are unchanged.
