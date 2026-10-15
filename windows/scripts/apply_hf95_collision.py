from pathlib import Path
import shutil
base=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
src=base/"src/HF94";dst=base/"src/HF95"
if dst.exists(): raise SystemExit("HF95 exists: abort")
shutil.copytree(src,dst,ignore=shutil.ignore_patterns("*.obj","*.o","build","*.exe"))
p=dst/"src/main.cpp"
s=p.read_text(encoding="utf-8")
def patch(a,b):
 global s
 assert s.count(a)==1,(a[:80],s.count(a))
 s=s.replace(a,b,1)
patch("    std::uint32_t mondainDefeatPopupStartedMs=0;","    bool mondainCollisionFlashPending=false;\n    std::uint32_t mondainDefeatPopupStartedMs=0;")
patch('''                        // Boundary, wall, gem, and Mondain collision all burn the avatar.
                        const int damage=''','''                        // MONDAIN.EXE 0x07B3: sound(0), negative palette,
                        // Burned + damage, sound(2), then restore palette.
                        if(noiseEnabled) sfx.play(0);
                        mondainCollisionFlashPending=true;
                        const int damage=''')
patch('''        if(mondainPsionicFramePresented) invertCurrentGameFrame();''','''        if(mondainPsionicFramePresented || mondainCollisionFlashPending)
            invertCurrentGameFrame();''')
patch('''        presentFrame();
        // OUT.EXE 0x108B''','''        presentFrame();
        if(mondainCollisionFlashPending){
            // The inverted frame must actually be presented while source sound(2)
            // runs; the normal frame returns on the next render iteration.
            if(noiseEnabled) sfx.play(2);
            mondainCollisionFlashPending=false;
        }
        // OUT.EXE 0x108B''')
p.write_text(s,encoding="utf-8")
print("HF95 collision flash/sound patch completed",p)