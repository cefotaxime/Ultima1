from pathlib import Path
import subprocess,sys,os,re,tempfile,json
root=Path(__file__).resolve().parents[1]
tools=root/"tools"
pkg=r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix42"
env=os.environ.copy(); env["PYTHONUTF8"]="1"; env["PYTHONIOENCODING"]="utf-8"
superseded={
"audit_hf20_drop_pence_localization.py":"later Drop-Pence localization",
"audit_hf24_ending_hold_scroll_stop.py":"later user-approved second-line ending stop (380)",
"audit_hf28_reachable_output_english.py":"HF42 deliberately localized defensive English fallbacks",
"audit_hf31_command_beep_matrix.py":"Open prompt later changed to 관 열기?",
}
files=sorted(tools.glob("audit_hf*.py"))
rows=[]
for f in files:
    src=f.read_text(encoding="utf-8",errors="ignore")
    patched=re.sub(r'C:\\Users\\hojun\\Downloads\\Ultima1_KR_1\.0K_Test_Hotfix\d+',pkg.replace("\\","\\\\"),src)
    tmp=tools/("_compat_"+f.name)
    tmp.write_text(patched,encoding="utf-8")
    p=subprocess.run([sys.executable,str(tmp)],cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=25,env=env)
    tmp.unlink(missing_ok=True)
    raw="PASS" if p.returncode==0 else "FAIL"
    verdict="PASS" if raw=="PASS" else ("SUPERSEDED" if f.name in superseded else "REVIEW")
    rows.append((f.name,raw,verdict,p.stdout[-3000:]))
print("TOTAL",len(rows),"PASS",sum(x[2]=="PASS" for x in rows),"SUPERSEDED",sum(x[2]=="SUPERSEDED" for x in rows),"REVIEW",sum(x[2]=="REVIEW" for x in rows))
for name,raw,v,out in rows:
    if v!="PASS":
        print(v,name)
        if v=="REVIEW": print(out[-1000:])
