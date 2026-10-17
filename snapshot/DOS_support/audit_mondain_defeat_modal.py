from pathlib import Path
p=Path(__file__).resolve().parents[1]/"DOS_RESUME_20261017_DIAG/source/main.cpp"
s=p.read_text(encoding="utf-8")
checks={
    "collision damage": "const int damage=std::max(1,static_cast<int>(p.hits)/10);",
    "modal death freezes command log": "log=logBeforeMondainMove;",
    "modal death suppresses cursor": "const std::string defeatSafeModalInput=(mondain.active && mondain.defeated)?std::string{}:modalInput;",
    "modal log uses safe input": "drawLog(r,font,original,log,defeatSafeModalInput,",
    "defeat timer": "MONDAIN_DEFEAT_POPUP_MS=2747",
    "render suppresses normal prompt": "!(mondain.active && mondain.defeated) &&",
}
for k,token in checks.items():
 assert token in s,k
print("DOS_MONDAIN_DEFEAT_STATIC_AUDIT_OK",len(checks))
