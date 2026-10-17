from pathlib import Path
import subprocess,sys,os,tempfile,re
root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF38")
tools=root/"tools"
names=[
"audit_hf29_space_crunch_sfx.py",
"audit_hf30_space_combat_sfx.py",
"audit_hf31_local_guard_sfx.py",
"audit_hf32_resurrection_sfx.py",
"audit_hf32_sound_call_inventory.py",
"audit_hf33_king_pence_sfx.py",
"audit_hf34_shop_afford_sfx.py",
]
env=os.environ.copy(); env["PYTHONUTF8"]="1"; env["PYTHONIOENCODING"]="utf-8"
for name in names:
    src=(tools/name).read_text(encoding="utf-8")
    src=re.sub(r'Ultima1_KR_1\.0K_Test_Hotfix\d+', 'Ultima1_KR_1.0K_Test_Hotfix38', src)
    tmp=tools/("_compat_"+name)
    tmp.write_text(src,encoding="utf-8")
    p=subprocess.run([sys.executable,str(tmp)],cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",env=env)
    print(("PASS" if p.returncode==0 else "FAIL"),name)
    if p.returncode: print(p.stdout[-2500:])
    tmp.unlink(missing_ok=True)
