from pathlib import Path
import subprocess, re, json, sys, time, os
root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF38")
tools=root/"tools"
gamedata=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix38\GAMEDATA")
items=[]
for f in tools.glob("audit_m*.py"):
    m=re.match(r"audit_m(\d+)", f.name)
    if m and 1<=int(m.group(1))<=238:
        items.append((int(m.group(1)),f))
items.sort(key=lambda x:(x[0],x[1].name))
env=os.environ.copy(); env["PYTHONUTF8"]="1"; env["PYTHONIOENCODING"]="utf-8"
results=[]
for n,f in items:
    txt=f.read_text(encoding="utf-8",errors="ignore")
    args=[sys.executable,str(f)]
    if "sys.argv" in txt or "usage:" in txt.lower(): args.append(str(gamedata))
    t=time.time()
    try:
        p=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                         text=True,encoding="utf-8",errors="replace",timeout=25,env=env)
        st="PASS" if p.returncode==0 else "FAIL"; out=p.stdout[-8000:]
    except subprocess.TimeoutExpired as e:
        st="TIMEOUT"; raw=e.stdout or ""; out=(raw.decode("utf-8","replace") if isinstance(raw,bytes) else raw)[-8000:]
    results.append({"m":n,"file":f.name,"status":st,"sec":round(time.time()-t,2),"out":out})
(tools/"full_audit_m1_m238_results_utf8.json").write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding="utf-8")
from collections import Counter
print("TOTAL",len(results),dict(Counter(r["status"] for r in results)))
print("FAILED",sum(r["status"]!="PASS" for r in results))