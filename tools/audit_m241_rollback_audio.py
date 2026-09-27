#!/usr/bin/env python3
from pathlib import Path
import hashlib,re
root=Path(__file__).resolve().parents[1]
expected={
    'src/PcSpeakerEmulator.cpp':'0cf4d0f219d79c8525f64b7c0787efc031dc5ae46652455020fd675ca0655126',
    'src/PcSpeakerEmulator.h':'b7f5b4f0663671975b98bd16abb30f7595fd50c866a57a77eca2c7e949beeb01',
}
allowed_sfx={
    'src/SfxSystem.cpp':{
        'b50462c851ff04629e90052c844d3fb98387280bdbd44d58996d3779d63bd987',
        '8669e0c8b1539d2f94db4f7ecd0b454e03fd6fb45555b7dcd705509993c6bce0',
    },
    'src/SfxSystem.h':{
        '583960ddc01d42aa1d1e56c05468a21cff54bf540fd1f4c58b266de4ea62101c',
        '24da09c06d7ef72a97afbd44eb2250283cfb72222731f5b278abe06376bcade6',
    },
}
for rel,want in expected.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest(); assert got==want,(rel,got,want)
for rel,allowed in allowed_sfx.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest(); assert got in allowed,(rel,got,allowed)
sfx=(root/'src/SfxSystem.cpp').read_text(encoding='utf-8')
for forbidden in ['pulsePlayedSampleLocked','pulseWriteActiveLocked','PA_SEEK_RELATIVE_ON_READ','PULSE_MAX_BURST_FRAMES','waitPulsePlayedThrough']:
    assert forbidden not in sfx,forbidden
for required in ['transport=M238-direct-pulse-write-callback','PULSE_TARGET_LATENCY_US=5000','waitRenderedThrough(sourceEndSample,us+1000000u);']:
    assert required in sfx,required
print('M241 rollback audio audit OK')
