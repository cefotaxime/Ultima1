# M225 DOSBox capture-oracle SFX

These WAV files are direct mono extractions from the user's DOSBox Staging 0.82.2 recordings using the supplied `dosbox.conf` (`core=auto`, real-mode `cpu_cycles=3000`, `pcspeaker=impulse`, `pcspeaker_filter=on`, mixer 48 kHz).

They are presentation PCM only. The audible filter tail is intentionally retained. Gameplay code must **not** use WAV length as the synchronous DOS `sound()` blocking time; M225 keeps that in a separate logical-duration table.

Mapping:

- 0: outdoor terrain/transport collision (`blocked_woods.wav`)
- 1: local blocked / invalid X-it (`Xit_unavailable.wav`)
- 2: combat hit / trap (`set_off_a_trap.wav`)
- 3: enemy ranged launch (`enemy attack.wav`, first pure launch)
- 4: foot/local movement (`footstep_field.wav`)
- 5: positive/success effect (`success.wav`)
- 6: failure/no-effect (`failure.wav`)
- 7: player normal Attack launch (`blaster.wav`)
- 8: Frigate/Aircar Fire (`airship_fire.wav`)
- 9: SPACE.EXE flight pulse (`space.wav`, one isolated pulse from the repeated pair cadence)

Fire correction: OUT.EXE Fire loads sound ID 8 for both Frigate and Aircar. M224's temporary ID 6 mapping was incorrect.
