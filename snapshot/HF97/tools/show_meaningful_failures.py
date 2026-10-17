import json
from pathlib import Path
R=json.loads(Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF38\tools\full_audit_m1_m238_results_utf8.json").read_text(encoding="utf-8"))
wanted={32,78,84,95,99,100,101,107,122,131,133,147,149,168,179,211}
for x in R:
    if x["m"] in wanted and x["status"]!="PASS":
        print("\n"+"="*80)
        print("M",x["m"],x["file"])
        print(x["out"][-3500:])
