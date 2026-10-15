# HF60 1986 ProDOS disk loader follow-up — 2026-10-09
Disk: research/ultima1986_u1_program.dsk. Analyzed ProDOS catalog directory records (script tools/hf60_disk_loader_audit.py).
Key binary load addresses (AUX type 06): SPA=$8956, STUPH=$0800, MAKE.INDATA=$1E00, NIF=$4000, TCMAPS=$4000, MAPCHARS=$0800. OUT/DNG/TWN/CAS/GEN/TM each load at $8956 and overlay SPA (mutually exclusive modules).
$8266 lies outside SPA image at $8956..$B01F and outside STUPH image at $0800..$1FFF. Catalog does not contain a separately declared file starting at $8266.
Normal original SPA controller: $9477 JSR $15C8; $947A JSR $9497; $947D JSR $8266. The synthetic harness can enter the first two calls, but lacks executable code at $8266.
Important unresolved: $8266 might be created by a boot-time relocation, loaded by a separate disk, populated by resident runtime not represented in current extracted program files, or otherwise derived. Not yet identified. Do not claim it is necessarily a specific file.
Original bootloader and runtime memory initialization must be reconstructed or observed before calling controller $9477 fully reproduced.
No game/audio code changed or package generated in this investigation.