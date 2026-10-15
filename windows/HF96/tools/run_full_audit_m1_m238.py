from pathlib import Path
import subprocess, re, json, sys, time

root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF38")
tools=root/"tools"
gamedata=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix38\GAMEDATA")
items=[]
for f in tools.glob("audit_m*.py"):
    m=re.match(r"audit_m(\d+)", f.name)
    if not m: continue
    n=int(m.group(1))
    if 1 <= n <= 238:
        items.append((n,f))
items.sort(key=lambda x:(x[0],x[1].name))

results=[]
for n,f in items:
    text=f.read_text(encoding="utf-8",errors="ignore")
    args=[sys.executable,str(f)]
    if "sys.argv" in text or "usage:" in text.lower():
        args.append(str(gamedata))
    t=time.time()
    try:
        p=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                         text=True,encoding="utf-8",errors="replace",timeout=25)
        status="PASS" if p.returncode==0 else "FAIL"
        out=p.stdout[-6000:]
    except subprocess.TimeoutExpired as e:
        status="TIMEOUT"
        raw=e.stdout or ""
        if isinstance(raw,bytes): raw=raw.decode("utf-8","replace")
        out=raw[-6000:]
    results.append({"m":n,"file":f.name,"status":status,"sec":round(time.time()-t,2),"out":out})

outf=tools/"full_audit_m1_m238_results.json"
outf.write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding="utf-8")
from collections import Counter
c=Counter(r["status"] for r in results)
print("TOTAL",len(results),dict(c))
for r in results:
    if r["status"]!="PASS":
        print(f'{r["status"]} M{r["m"]} {r["file"]} {r["sec"]}s')
        lines=[x for x in r["out"].splitlines() if x.strip()]
        for x in lines[-4:]:
            print("   ",x[:500])