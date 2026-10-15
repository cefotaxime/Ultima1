from pathlib import Path
import shutil
root=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
src=root/"src/HF95"; dst=root/"src/HF96"
assert src.is_dir() and not dst.exists()
shutil.copytree(src,dst)
for filename in ("main.cpp","main_bilinear.cpp"):
 p=dst/"src"/filename
 s=p.read_text(encoding="utf-8")
 for a,b,count in [("무제한 휴대식량","무제한 휴대 식량",1),("드라시산 갱도","드래시산 갱도",2)]:
  assert s.count(a)==count,(filename,a,s.count(a))
  s=s.replace(a,b)
 p.write_text(s,encoding="utf-8")
 print(filename,"APPLIED 3 changes")
for p in (dst/"src").glob("*.cpp"):
 s=p.read_text(encoding="utf-8")
 assert not any(v in s for v in ("휴대식량","드라시산","우주기지","등록상표")),(p.name,"old terms remain")
print("PASS: source verified; no build or packaging")