from pathlib import Path
import re,subprocess,os,json,time

root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF42")
cm=(root/"CMakeLists.txt").read_text(encoding="utf-8")
build=root/"build_all_tests_direct"
pkg=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix42")
gamedata=pkg/"GAMEDATA"
fresh_gamedata=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_HF42_FreshFixture\GAMEDATA")
env=os.environ.copy()
env["SDL_VIDEODRIVER"]="dummy"
env["SDL_AUDIODRIVER"]="dummy"

targets=[]
for m in re.finditer(r'add_executable\s*\(\s*([A-Za-z0-9_]+)\s+([\s\S]*?)\)',cm):
    name=m.group(1); body=m.group(2)
    srcs=[x.replace("/","\\") for x in re.findall(r'[^\s#]+',body) if x.endswith(".cpp")]
    if srcs: targets.append((name,srcs))

skip={"u1kr_clean","u1kr_tile_inspector"}
rows=[]
for name,srcs in targets:
    if name in skip: continue
    exe=build/(name+".exe")
    if not exe.exists():
        rows.append({"target":name,"status":"NO_EXE","out":""}); continue
    toolsrc=None
    for s in srcs:
        if s.lower().startswith("tools\\"):
            toolsrc=root/s; break
    txt=toolsrc.read_text(encoding="utf-8",errors="ignore") if toolsrc and toolsrc.exists() else ""
    argsets=[[]]
    if name in {"u1kr_campaign_smoke","u1kr_steal_route_m40"}:
        argsets=[[str(fresh_gamedata)]]
    elif "argc" in txt or "argv" in txt or "usage:" in txt.lower():
        argsets=[[str(gamedata)],[str(pkg)],[str(gamedata/"OUT.EXE")],[]]
    best=None
    for extra in argsets:
        try:
            t=time.time()
            p=subprocess.run([str(exe)]+extra,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                             text=True,encoding="utf-8",errors="replace",timeout=20,env=env)
            cand={"target":name,"status":"PASS" if p.returncode==0 else "FAIL","args":extra,
                  "sec":round(time.time()-t,2),"out":p.stdout[-5000:]}
        except subprocess.TimeoutExpired as e:
            cand={"target":name,"status":"TIMEOUT","args":extra,"sec":20,"out":""}
        if cand["status"]=="PASS":
            best=cand; break
        if best is None: best=cand
    rows.append(best)
    print(best["status"],name,"args=",best.get("args"))
    if best["status"]!="PASS":
        print("\n".join(best["out"].splitlines()[-12:]))

(root/"tools"/"run_all_compiled_regressions_results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
from collections import Counter
print("SUMMARY",dict(Counter(x["status"] for x in rows)))
