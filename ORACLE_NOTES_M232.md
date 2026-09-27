> **Superseded by M233:** the 32-frame / 250 us / Pulse 2 ms transport experiment increased observed lag from about 4 to about 7.5 steps. M233 restores the M231 transport while retaining the M232 sound(4) pitch correction.

# M232 — remaining application-side latency reduction + sound(4) pitch correction

M231 removed idle-silence prefill completely, but the application still requested a 64-frame SDL period (1.333 ms at 48 kHz) and slept 1 ms between feeder pumps. M232 requests 32 frames (0.667 ms), uses a 250 us active feeder cadence, and blocks the feeder when no voice exists. PulseAudio latency request is reduced from 5 ms to 2 ms.

M230/M231 also lowered the footstep pitch by stretching sound(4)'s toggle train across the full 4.302 ms source span. M232 preserves the 4.302 ms synchronous source duration but calibrates the audible toggle train against the M225 direct DOSBox capture: first significant sample at sample 12, then roughly 21-22 samples between toggles. Offline probe of the actual generated PCM gives onset 12 vs capture 12 and dominant 1125 Hz FFT bin vs capture 1125 Hz.

This does not claim that WSLg downstream presentation latency is eliminated. U1_SFX_TRACE=1 remains the diagnostic oracle if a multi-step audible lag remains.
