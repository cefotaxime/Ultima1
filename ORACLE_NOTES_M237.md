# M237 — Windows minimum-period event-driven WASAPI

M236 hotfix1 native Windows runtime reduced the user's first-footstep audible onset from roughly six steps to roughly three steps while preserving the restored PC-speaker sound quality. M237 therefore leaves the PC-speaker synthesis untouched and changes only the native Windows output transport.

On Windows, M237 prefers an IAudioClient3 shared stream using the minimum period returned by GetSharedModeEnginePeriod, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, and IAudioRenderClient. The render thread requests the MMCSS "Pro Audio" task class when available.

The low-latency path accepts only a 48 kHz Windows mix format. If the endpoint uses another engine rate or low-latency initialization fails, the game automatically falls back to the proven M236 SDL/WASAPI direct-callback path. U1_SFX_FORCE_SDL=1 forces that fallback for A/B testing.

There is still no runtime WAV playback, SDL_QueueAudio, feeder thread, application PCM queue, or idle-silence prefill.

The M237 audit locks src/PcSpeakerEmulator.cpp to the M236 hotfix1 SHA-256:

0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126

so latency work cannot silently change the approved waveform/filter model.
