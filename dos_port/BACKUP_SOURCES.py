
from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
import hashlib
root=Path(__file__).resolve().parents[1]
out=root/'dos_port'/'ULTIMA1_KR_SOURCE_RECOVERY.zip'
roots=[root/'src'/'HF93'/'src',root/'dos_port'/'V1_0K']
excluded={'.zip','.bak','.o','.exe','.com','.wav','.log','.ok','.txt'}
count=0
with ZipFile(out,'w',compression=ZIP_DEFLATED,compresslevel=6) as z:
 for base in roots:
  if not base.exists():raise FileNotFoundError(base)
  for p in base.rglob('*'):
   if p.is_file() and p.suffix.lower() not in excluded:
    z.write(p,p.relative_to(root));count+=1
 for name in ('dos_port/BUILD_TEST_PACKAGE.ps1','dos_port/PACKAGE_DOS.py','dos_port/DOS_PORT_HANDOFF.md','dos_port/BACKUP_SOURCES.py'):
  p=root/name
  if p.exists():z.write(p,name);count+=1
print('SOURCE BACKUP SUCCESS',out,'file_count',count,'bytes',out.stat().st_size,'sha256',hashlib.sha256(out.read_bytes()).hexdigest())
