
from pathlib import Path
import hashlib,zipfile
root=Path(__file__).resolve().parent
game=root/'V1_0K'
out=root/'ULTIMA1_KR_DOS_VERIFIED_HANDOFF.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
 for f in game.iterdir():
  if f.is_file() and f.suffix.lower()!='.bak':
   z.write(f,'V1_0K/'+f.name)
 for name in ('BUILD_TEST_PACKAGE.ps1','PACKAGE_DOS.py','dosbox_move_sound_validation.conf','dosbox_collision_validation.conf','dosbox_water_validation.conf','dosbox_save_validation.conf','DOS_PORT_HANDOFF.md'):
  f=root/name
  if f.exists():z.write(f,name)
print('SUCCESS',out,'bytes',out.stat().st_size,'sha256',hashlib.sha256(out.read_bytes()).hexdigest())
