from pathlib import Path
import re, subprocess, json, os, shlex, time, sys

root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF42")
cm=(root/"CMakeLists.txt").read_text(encoding="utf-8")
compiler=r"C:\Users\hojun\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260616-ucrt-x86_64\bin\clang++.exe"
sdl=Path(r"C:\Users\hojun\Downloads\SDL2-2.30.9\x86_64-w64-mingw32")
outdir=root/"build_all_tests_direct"
outdir.mkdir(exist_ok=True)

targets=[]
for m in re.finditer(r'add_executable\s*\(\s*([A-Za-z0-9_]+)\s+([\s\S]*?)\)',cm):
    name=m.group(1)
    body=m.group(2)
    srcs=[]
    for tok in re.findall(r'[^\s#]+',body):
        if tok.endswith(".cpp"):
            srcs.append(tok.replace("/","\\"))
    if srcs: targets.append((name,srcs))

defs={}
for m in re.finditer(r'target_compile_definitions\s*\(\s*([A-Za-z0-9_]+)\s+PRIVATE\s+([\s\S]*?)\)',cm):
    defs[m.group(1)]=[x for x in re.findall(r'[^\s#]+',m.group(2)) if not x.startswith("$")]

wanted=set(sys.argv[1:])
if wanted: targets=[x for x in targets if x[0] in wanted]
rows=[]
for idx,(name,srcs) in enumerate(targets,1):
    exe=outdir/(name+".exe")
    args=[compiler,"-std=c++17","-O0","-DSDL_MAIN_HANDLED","-D_WIN32_WINNT=0x0A00","-DWINVER=0x0A00",
          "-I"+str(root/"src"),"-I"+str(sdl/"include"/"SDL2")]
    for d in defs.get(name,[]): args.append("-D"+d)
    args += [str(root/s) for s in srcs]
    args += ["-L"+str(sdl/"lib"),"-lSDL2","-lole32","-luuid","-lavrt","-lksuser","-o",str(exe)]
    t=time.time()
    p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=120)
    rows.append({"target":name,"ok":p.returncode==0,"sec":round(time.time()-t,2),"out":p.stdout[-6000:]})
    print(f"[{idx}/{len(targets)}]", "PASS" if p.returncode==0 else "FAIL", name)
    if p.returncode:
        print("\n".join(p.stdout.splitlines()[-12:]))

(root/"tools"/"direct_compile_all_targets_results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding="utf-8")
print("TOTAL",len(rows),"PASS",sum(x["ok"] for x in rows),"FAIL",sum(not x["ok"] for x in rows))
