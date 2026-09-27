#!/usr/bin/env python3
from pathlib import Path
import hashlib,re
root=Path(__file__).resolve().parents[1]
fixed={
    'src/PcSpeakerEmulator.cpp':'0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126',
    'src/PcSpeakerEmulator.h':'b7f5b4f0663671975b98bd16abb30f7595fd50c866a57a77eca2c7e949beeb01',
}
allowed={
    'src/SfxSystem.cpp':{
        'b50462c851ff04629e90052c844d3fb98387280bdbd44d58996d3779d63bd987',
        '551bfd242012cd80e3a30981a8c9b33941f696d9dec5f35fbdac76ef5463ca2c',
    },
    'src/SfxSystem.h':{
        '583960ddc01d42aa1d1e56c05468a21cff54bf540fd1f4c58b266de4ea62101c',
        'c50ae46a11a136019960c95bcddfc15dfbb70a04dababf6402537048d3f40a68',
    },
}
for rel,want in fixed.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest(); assert got==want,(rel,got,want)
for rel,wants in allowed.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest(); assert got in wants,(rel,got,wants)
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
for forbidden in ['waitPulsePlayedThrough','PA_SEEK_RELATIVE_ON_READ','pulsePlayedSampleLocked','pulseWriteActiveLocked','PULSE_MAX_BURST_FRAMES']:
    assert forbidden not in sfx,forbidden
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
m=re.search(r'Ultima I KR clean-room M(\d+)',main); assert m and int(m.group(1))>=243
pm=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cm); assert pm and int(pm.group(1))>=243
print('M243/M245 M241-hotfix1 baseline-preservation audit OK')
