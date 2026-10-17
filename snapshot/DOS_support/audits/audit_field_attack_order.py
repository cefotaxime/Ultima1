from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
main=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
sim=(r/"src/HF93/src/OverworldSimulation.cpp").read_text(encoding="utf-8")
checks={
 "ATTACK_RESOLUTION_AFTER_PROJECTILE":main.index("if(worldProjectileIndex>=worldProjectilePath.size())")<main.index("if(enemyAttackVisualActive){"),
 "PROJECTILE_FINISH_CALLS_RESOLVE":"if(worldProjectileIndex>=worldProjectilePath.size()){\n                    resolveDelayedAttack();" in main,
 "WORLD_ATTACK_EXECUTED_IN_RESOLVE":"overworld.attack(p,pendingAttackDx,pendingAttackDy,WorldAttackMode::Normal)" in main,
 "MISS_BEFORE_DAMAGE":sim.index("if(sharedRng_.randomRange(1,100)>hitChance)")<sim.index("lastPlayerAttackHit_=true;"),
 "HIT_LOG_BEFORE_KILL_ERASE":sim.index("out.push_back(result);",sim.index("// M119 presentation"))<sim.index("eraseObject(p,oi);",sim.index("// M119 presentation")),
 "KILL_ROW_AFTER_HIT_ROW":sim.index("eraseObject(p,oi);",sim.index("// M119 presentation"))<sim.index("out.push_back(killReward);",sim.index("// M119 presentation")),
 "NO_EARLY_ERASE":sim[sim.index("std::vector<std::string> OverworldSimulation::attack("):sim.index("// M119 presentation")].count("eraseObject(")==0,
}
for k,v in checks.items():print(("PASS" if v else "FAIL")+" "+k)
assert all(checks.values())