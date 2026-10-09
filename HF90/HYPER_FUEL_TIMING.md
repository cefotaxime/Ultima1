# HF90 — Apple II hyperjump fuel deduction timing

Based on HF89. Apple II 1986 SPA `$9842 LDA #$64 / $9844 JSR $A154` deducts 100 fuel after acceleration, before lightspeed. HF90 deducts 100 once on transition to stage 2, in both CPU-audio-synchronized `updateAppleHyperProgress` and fallback `updateHyperJump` paths. The former completion-stage deduction has been removed, to prevent double charge. Fuel under 100 still refuses to start. Windows MinGW build succeeded; actual gameplay smoke test remains pending.

Remote package: `C:\Users\hojun\Downloads\Ultima1_KR_Work\packages\HF90_APPLE_HYPER_FUEL_TIMING.zip`.
