from pathlib import Path
import subprocess, re, json, sys, os, time
from collections import Counter

root=Path(__file__).resolve().parents[1]
tools=root/"tools"
downloads=Path(r"C:\Users\hojun\Downloads")
packages=[]
for p in downloads.glob("Ultima1_KR_1.0K_Test_Hotfix*"):
    m=re.search(r"Hotfix(\d+)$",p.name)
    if m and (p/"GAMEDATA"/"OUT.EXE").exists(): packages.append((int(m.group(1)),p))
assert packages, "No usable Ultima1_KR_1.0K_Test_Hotfix*/GAMEDATA package found"
gamedata=max(packages,key=lambda x:x[0])[1]/"GAMEDATA"
manifest=json.loads((tools/"full_regression_manifest_1_0K.json").read_text(encoding="utf-8"))
superseded=manifest["superseded"]

items=[]
for f in tools.glob("audit_m*.py"):
    m=re.match(r"audit_m(\d+)",f.name)
    if m and 1<=int(m.group(1))<=238:
        items.append((int(m.group(1)),f))
items.sort(key=lambda x:(x[0],x[1].name))

env=os.environ.copy()
env["PYTHONUTF8"]="1"
env["PYTHONIOENCODING"]="utf-8"
rows=[]
for n,f in items:
    txt=f.read_text(encoding="utf-8",errors="ignore")
    args=[sys.executable,str(f)]
    if "sys.argv" in txt or "usage:" in txt.lower():
        args.append(str(gamedata))
    try:
        p=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                         text=True,encoding="utf-8",errors="replace",timeout=25,env=env)
        raw="PASS" if p.returncode==0 else "FAIL"
        out=p.stdout[-5000:]
    except subprocess.TimeoutExpired as e:
        raw="TIMEOUT"
        x=e.stdout or ""
        out=(x.decode("utf-8","replace") if isinstance(x,bytes) else x)[-5000:]
    if raw=="PASS":
        verdict="PASS"
    elif f.name in superseded:
        verdict="SUPERSEDED"
    else:
        verdict="REVIEW"
    rows.append({"m":n,"file":f.name,"raw":raw,"verdict":verdict,"output":out,
                 "reason":superseded.get(f.name,"")})

summary=Counter(r["verdict"] for r in rows)
print("SUMMARY",dict(summary))
print("MISSING_AUDIT_NUMBERS",manifest["missing_numbers"])
for r in rows:
    if r["verdict"]!="PASS":
        print(f'{r["verdict"]}: M{r["m"]} {r["file"]}')
        if r["reason"]: print("  ",r["reason"])
(tools/"full_regression_last_results_1_0K.json").write_text(
    json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
if summary.get("REVIEW",0):
    raise SystemExit(2)
