import json
from pathlib import Path
from collections import Counter
p=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF38\tools\full_audit_m1_m238_results_utf8.json")
R=json.loads(p.read_text(encoding="utf-8"))
cats=Counter(); ex={}
for x in R:
    if x["status"]=="PASS": continue
    o=x["out"]
    if "Ultima I KR clean-room M" in o:
        k="STALE_TITLE_VERSION"
    elif "int(m.group(1)) >= at_least" in o or "mver and int(mver.group(1))" in o:
        k="STALE_VERSION_PARSE"
    elif "FileNotFoundError" in o:
        k="MISSING_DEPENDENCY"
    elif "AssertionError" in o:
        k="ASSERT_OTHER"
    elif "SystemExit:" in o:
        k="SYSTEMEXIT_OTHER"
    else:
        k="OTHER"
    cats[k]+=1; ex.setdefault(k,[]).append((x["m"],x["file"]))
print(dict(cats))
for k,v in ex.items():
    print("\n"+k, len(v))
    print(v)
