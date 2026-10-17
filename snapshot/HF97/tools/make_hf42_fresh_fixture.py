from pathlib import Path
import shutil, struct
src=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Test_Hotfix42\GAMEDATA")
dst=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_HF42_FreshFixture\GAMEDATA")
if dst.parent.exists(): shutil.rmtree(dst.parent)
shutil.copytree(src,dst)
orig=(src/"PLAYER1.U1").read_bytes()
b=bytearray(820)
# preserve legacy name from the current fixture, but reset every gameplay byte.
b[:16]=orig[:16]
def wr(o,v):
    b[o:o+2]=int(v&0xffff).to_bytes(2,"little")
wr(0x10,0); wr(0x12,3); wr(0x14,0) # Human male Thief: historical M39 stock fixture
wr(0x16,150); wr(0x18,16); wr(0x1A,26); wr(0x1C,16); wr(0x1E,16); wr(0x20,13); wr(0x22,18)
wr(0x24,100); wr(0x26,0); wr(0x28,200)
wr(0x2A,1); wr(0x2C,0); wr(0x2E,1); wr(0x30,0)
# RNG seed is generation-time state and is not mutated by saveState; preserve historical seed.
seed=orig[0x32] | (orig[0x33]<<8); wr(0x32,seed)
wr(0x34,49); wr(0x36,40); wr(0x38,1)
# quests/gems remain zero
wr(0x54,0xffff); wr(0x56,1)
wr(0x60,0xffff); wr(0x62,2)
wr(0x80,0xffff)
wr(0x96,0xffff)
wr(0xA6,0); wr(0xA8,0xffff); wr(0xAC,0); wr(0xAE,0); wr(0xB0,0); wr(0xB2,0)
# player world object only
for j,v in enumerate([10,1,49,40,0,0,0,0]): wr(0xB4+j*2,v)
(dst/"PLAYER1.U1").write_bytes(b)
print("fresh fixture",dst,"seed",seed,"bytes",len(b))
