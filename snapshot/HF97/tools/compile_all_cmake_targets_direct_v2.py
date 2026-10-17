from pathlib import Path
import re, subprocess, json, time

root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF42")
lines=(root/"CMakeLists.txt").read_text(encoding="utf-8").splitlines()
compiler=r"C:\Users\hojun\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260616-ucrt-x86_64\bin\clang++.exe"
sdl=Path(r"C:\Users\hojun\Downloads\SDL2-2.30.9\x86_64-w64-mingw32")
outdir=root/"build_all_tests_direct_v2"; outdir.mkdir(exist_ok=True)

# Parse add_executable blocks line-by-line to avoid regex stopping at nested syntax/comments.
targets=[]
i=0
while i<len(lines):
    line=lines[i].strip()
    if line.startswith("add_executable("):
        buf=[line]; depth=line.count("(")-line.count(")")
        i+=1
        while i<len(lines) and depth>0:
            buf.append(lines[i]); depth += lines[i].count("(")-lines[i].count(")"); i+=1
        block="\n".join(buf)
        inner=block[block.find("(")+1:block.rfind(")")]
        toks=[]
        for raw in inner.split():
            t=raw.strip()
            if t.startswith("#"): break
            toks.append(t)
        if toks:
            name=toks[0]
            srcs=[t for t in toks[1:] if t.endswith(".cpp")]
            if srcs: targets.append((name,srcs))
        continue
    i+=1

print("TARGETS",len(targets))
rows=[]
for idx,(name,srcs) in enumerate(targets,1):
    exe=outdir/(name+".exe")
    args=[compiler,"-std=c++17","-O0","-DSDL_MAIN_HANDLED","-D_WIN32_WINNT=0x0A00","-DWINVER=0x0A00",
          "-I"+str(root/"src"),"-I"+str(sdl/"include"/"SDL2")]
    # Special historical compile definition still required by campaign smoke.
    if name=="u1kr_campaign_smoke": args.append("-DU1_CAMPAIGN_DUNGEON_TIME_HASH=3")
    args += [str(root/s.replace("/","\\")) for s in srcs]
    args += ["-L"+str(sdl/"lib"),"-lSDL2","-lole32","-luuid","-lavrt","-lksuser","-o",str(exe)]
    t0=time.time()
    try:
        p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=120)
        ok=p.returncode==0; out=p.stdout[-7000:]
    except subprocess.TimeoutExpired as e:
        ok=False; out="TIMEOUT"
    rows.append({"target":name,"ok":ok,"sec":round(time.time()-t0,2),"out":out})
    print(f"[{idx}/{len(targets)}] {'PASS' if ok else 'FAIL'} {name}", flush=True)

(root/"tools"/"direct_compile_all_targets_results_v2.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
print("TOTAL",len(rows),"PASS",sum(x["ok"] for x in rows),"FAIL",sum(not x["ok"] for x in rows))
for x in rows:
    if not x["ok"]:
        print("\nFAIL",x["target"]); print("\n".join(x["out"].splitlines()[-12:]))
