# M242 — measured Pulse playback fence

M241 hotfix1 is the user-validated audio-quality baseline. M242 does not alter PcSpeakerEmulator, PCM samples, Pulse buffer attributes, callback rendering, stream write position, pitch, filter, or source duration.

The existing callback fence still waits until the source-end sample has been rendered into Pulse. On the canonical WSL direct-Pulse path, M242 additionally queries pa_stream_get_latency() and estimates the current playback/read sample as renderedSamples minus latencyFrames. play() returns only after that measured playback cursor crosses the same source-end sample.

If Pulse timing is unavailable, M242 immediately retains M241 behavior rather than inventing a delay. M239/M240 timeline rewrite/seek logic remains excluded.
