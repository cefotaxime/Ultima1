# M233 — restore M231 transport, retain M232 footstep pitch

User runtime evidence is the deciding oracle for this change: M231 reduced the delayed/bunched footstep symptom to about four steps, whereas M232's smaller SDL period/faster feeder/lower Pulse request worsened it to about 7.5 steps.

M233 therefore restores the M231 application transport exactly: `REQUESTED_SAMPLES=64`, `FEEDER_SLEEP_MS=1`, and `PULSE_LATENCY_MSEC=5`. The M231 fix that prevents unconditional idle-silence prefill remains (`pumpLocked()` returns when there is no future voice).

M232's independent sound(4) pitch correction is retained. The audible toggle train is calibrated against the M225 direct DOSBox capture rather than stretched across the entire 4.302 ms source blocking span. The synchronous source span remains 4.302 ms.

M233 makes no claim that the remaining WSLg downstream presentation delay is solved. `U1_SFX_TRACE=1` remains available for measurement.
