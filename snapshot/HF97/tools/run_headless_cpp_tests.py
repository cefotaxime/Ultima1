from pathlib import Path
import subprocess, json, os, re, time

root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF42")
bindir=root/"build_all_tests_direct_v2"
tools=root/"tools"
pkg=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix42")
gamedata=pkg/"GAMEDATA"

skip={"u1kr_clean","u1kr_tile_inspector"}
rows=[]
env=os.environ.copy()
env["SDL_VIDEODRIVER"]="dummy"
env["SDL_AUDIODRIVER"]="dummy"
env["PATH"]=str(pkg)+";"+str(Path(r"C:\Users\hojun\Downloads\SDL2-2.30.9\x86_64-w64-mingw32\bin"))+";"+env.get("PATH","")

# Reconstruct target -> first source to infer argument expectations.
cm=(root/"CMakeLists.txt").read_text(encoding="utf-8")
target_sources={}
for m in re.finditer(r'add_executable\s*\(\s*([A-Za-z0-9_]+)\s+([\s\S]*?)\)',cm):
    name=m.group(1)
    srcs=re.findall(r'([A-Za-z0-9_./\\-]+\.cpp)',m.group(2))
    if srcs: target_sources[name]=srcs[0]

for exe in sorted(bindir.glob("*.exe")):
    name=exe.stem
    if name in skip: continue
    srcname=target_sources.get(name,"")
    src=(root/srcname).read_text(encoding="utf-8",errors="ignore") if srcname and (root/srcname).exists() else ""
    args=[str(exe)]
    if "GAMEDATA" in src or "argc" in src or "argv" in src:
        args.append(str(gamedata))
    t=time.time()
    try:
        p=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=20,env=env)
        status="PASS" if p.returncode==0 else "FAIL"
        out=p.stdout[-6000:]
    except subprocess.TimeoutExpired as e:
        status="TIMEOUT"; out=""
    rows.append({"target":name,"status":status,"sec":round(time.time()-t,2),"out":out})
    print(status,name,flush=True)
(tools/"runtime_headless_tests_results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
print("TOTAL",len(rows),"PASS",sum(x["status"]=="PASS" for x in rows),"FAIL",sum(x["status"]=="FAIL" for x in rows),"TIMEOUT",sum(x["status"]=="TIMEOUT" for x in rows))
for x in rows:
    if x["status"]!="PASS":
        print("\n"+x["status"],x["target"]); print("\n".join(x["out"].splitlines()[-10:]))
