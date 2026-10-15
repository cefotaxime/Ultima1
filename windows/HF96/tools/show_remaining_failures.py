import json
from pathlib import Path
R=json.loads(Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF38\tools\full_audit_m1_m238_results_utf8.json").read_text(encoding="utf-8"))
for x in R:
    if x["status"]!="PASS":
        print("\n"+"="*80)
        print("M",x["m"],x["file"])
        print(x["out"][-3000:])