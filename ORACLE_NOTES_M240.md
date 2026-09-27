# M240 — PulseAudio playback read-clock reanchor

M239 removed continuous idle zero-PCM writes and primed new sounds at the Pulse read cursor, but its application sample clock could still remain tied to the old write cursor.

M240 keeps the M236 hotfix1 synthesis byte-for-byte unchanged. At each new WSL sound, pa_stream_get_latency() maps the current application write position back to the Pulse playback/read position. renderedSamples_ is reanchored to that logical sample, still-live filter tails are mixed from the same point, and the first at-most-5-ms block is written with PA_SEEK_RELATIVE_ON_READ. Later callbacks append only active voice/tail data.

The Pulse stream uses prebuf=0 plus PA_STREAM_START_CORKED and one explicit initial uncork, so idle underruns do not rebuild a silent prebuffer before the next sparse PC-speaker effect.
