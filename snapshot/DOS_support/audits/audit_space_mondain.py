from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
s=(r/"src/HF93/src/SpaceSystem.cpp").read_text(encoding="utf-8")
m=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
checks={
"space_enter":"void SpaceSystem::enter(" in s,
"sector_scan_draw":"void SpaceSystem::drawSectorScan(" in s,
"sector_scan_activation":"s.sectorScan=true;" in s,
"hyperjump_start":"void SpaceSystem::hyperJump(" in s,
"hyperjump_four_phases":all("s.hyperPhase="+str(i)+";" in s for i in (1,2,3,4)),
"hyperjump_restore_idle":"s.hyperPhase=0;" in s,
"space_combat_fire":"void SpaceSystem::fire(" in s,
"space_draw":"drawSectorScan(r,s);" in s,
"mondain_test_entry":"--test-mondain-intro" in m,
"mondain_enter":"mondainSystem.enter(mondain,p,mm);" in m,
"mondain_render":"mondain.active" in m and "mondainIntroBlocking" in m,
}
lines=[("PASS " if v else "FAIL ")+k for k,v in checks.items()]
print("\n".join(lines))
assert all(checks.values())