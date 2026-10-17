from pathlib import Path
import subprocess,sys,os,re,json
root=Path(__file__).resolve().parents[1]
tools=root/"tools"
pkg=r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix42"
env=os.environ.copy(); env["PYTHONUTF8"]="1"; env["PYTHONIOENCODING"]="utf-8"
items=[]
for f in tools.glob("audit_m*.py"):
    m=re.match(r"audit_m(\d+)",f.name)
    if m and int(m.group(1))>238: items.append((int(m.group(1)),f))
items.sort(key=lambda x:(x[0],x[1].name))
rows=[]
for n,f in items:
    src=f.read_text(encoding="utf-8",errors="ignore")
    patched=re.sub(r'C:\\Users\\hojun\\Downloads\\Ultima1_KR_1\.0K_Test_Hotfix\d+',pkg.replace("\\","\\\\"),src)
    tmp=tools/("_late_"+f.name); tmp.write_text(patched,encoding="utf-8")
    try:
        args=[sys.executable,str(tmp)]
        if "sys.argv" in patched or "usage:" in patched.lower(): args.append(str(Path(pkg)/"GAMEDATA"))
        p=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=25,env=env)
        st="PASS" if p.returncode==0 else "FAIL"; out=p.stdout[-5000:]
    except subprocess.TimeoutExpired as e:
        st="TIMEOUT"; out=""
    tmp.unlink(missing_ok=True)
    rows.append({"m":n,"file":f.name,"status":st,"out":out})
print("TOTAL",len(rows),"PASS",sum(x["status"]=="PASS" for x in rows),"FAIL",sum(x["status"]!="PASS" for x in rows))
for x in rows:
    if x["status"]!="PASS":
        print("\n",x["status"],"M"+str(x["m"]),x["file"])
        lines=[q for q in x["out"].splitlines() if q.strip()]
        print("\n".join(lines[-6:]))
(tools/"late_m239_plus_results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
