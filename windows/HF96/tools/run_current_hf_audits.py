from pathlib import Path
import subprocess,sys,os,json,time
root=Path(__file__).resolve().parents[1]
tools=root/"tools"
downloads=Path(r"C:\Users\hojun\Downloads")
packages=[]
for p in downloads.glob("Ultima1_KR_1.0K_Test_Hotfix*"):
    import re
    m=re.search(r"Hotfix(\d+)$",p.name)
    if m and (p/"GAMEDATA"/"OUT.EXE").exists(): packages.append((int(m.group(1)),p))
assert packages, "No usable test package with GAMEDATA"
gamedata=max(packages,key=lambda x:x[0])[1]/"GAMEDATA"
files=sorted(list(tools.glob("audit_hf*.py"))+[tools/"audit_shift_h_v_restoration.py",tools/"audit_windows_video_config_restore.py",tools/"audit_known_remake_bugs_policy.py"])
env=os.environ.copy(); env["PYTHONUTF8"]="1"; env["PYTHONIOENCODING"]="utf-8"
rows=[]
for f in files:
    txt=f.read_text(encoding="utf-8",errors="ignore")
    args=[sys.executable,str(f)]
    if "sys.argv" in txt or "usage:" in txt.lower(): args.append(str(gamedata))
    try:
        p=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=25,env=env)
        st="PASS" if p.returncode==0 else "FAIL"; out=p.stdout[-5000:]
    except subprocess.TimeoutExpired as e:
        st="TIMEOUT"; out=""
    rows.append({"file":f.name,"status":st,"out":out})
print("TOTAL",len(rows),"PASS",sum(x["status"]=="PASS" for x in rows),"FAIL",sum(x["status"]!="PASS" for x in rows))
for x in rows:
    if x["status"]!="PASS":
        print("\n",x["status"],x["file"])
        print(x["out"][-1500:])
(tools/"current_hf_audit_results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")