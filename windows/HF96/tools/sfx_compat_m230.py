from pathlib import Path
import re


def project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def read_runtime(root: Path | None = None):
    root = root or project_root()
    return {
        'root': root,
        'main': (root/'src/main.cpp').read_text(encoding='utf-8'),
        'sfx': (root/'src/SfxSystem.cpp').read_text(encoding='utf-8'),
        'sfxh': (root/'src/SfxSystem.h').read_text(encoding='utf-8'),
        'pc': (root/'src/PcSpeakerEmulator.cpp').read_text(encoding='utf-8'),
        'pch': (root/'src/PcSpeakerEmulator.h').read_text(encoding='utf-8'),
        'ora': (root/'src/SfxOracle.h').read_text(encoding='utf-8'),
        'cmake': (root/'CMakeLists.txt').read_text(encoding='utf-8'),
        'build': (root/'build_and_run_wsl.sh').read_text(encoding='utf-8'),
    }


def assert_milestone(text: str, at_least: int = 230):
    m = re.search(r'Ultima I KR clean-room M(\d+)', text)
    assert m and int(m.group(1)) >= at_least


def assert_m230_runtime(rt):
    main,sfx,sfxh,pc,pch,cmake = rt['main'],rt['sfx'],rt['sfxh'],rt['pc'],rt['pch'],rt['cmake']
    assert_milestone(main,230)
    m=re.search(r'project\(ultima1_kr_clean_m(\d+) LANGUAGES CXX\)',cmake)
    assert m and int(m.group(1))>=230

    # Runtime source is the reconstructed OUT.EXE PC-speaker program, not one
    # of the historical per-ID WAV/PCM blobs.
    assert '#include "PcSpeakerEmulator.h"' in sfxh
    assert 'PcSpeakerEmulator speaker_' in sfxh
    assert 'src/PcSpeakerEmulator.cpp' in cmake
    assert 'src/SfxPcmOracleM225.cpp' not in cmake.split('add_executable(u1kr_clean',1)[1].split(')',1)[0]
    assert 'SfxPcmOracleM225' not in sfx

    # M230 introduced the reconstructed source program and synchronous play().
    # M234 keeps that source contract but removes the application queue entirely
    # and lets SDL pull samples directly through a device callback.
    assert 'const auto& clip=speaker_.clip(soundId)' in sfx
    queued = ('SDL_QueueAudio(device_,chunk.data()' in sfx and
              'while(feederRun_ && enabled_ && playedSamplesLocked()<endSample)' in sfx)
    callback = ('want.callback=&SfxSystem::audioCallback;' in sfx and
                'transport=SDL-direct-callback' in sfx and
                'app-queue=none' in sfx and
                (('waitSourceDurationUs(us);' in sfx) or
                 ('waitRenderedThrough(sourceEndSample,us+1000000u);' in sfx)))
    assert queued or callback
    if queued:
        assert 'if(!hasFutureVoice) return;' in sfx

    # The source topology is generated from the exact software-loop shapes.
    for q in [
        'w.assign(16,1000)', 'w.assign(48,355)',
        'const int count=(id==2?200:(id==3?255:8))',
        'id==5?0xc0:0x05', 'id==5?0x3c:0x0f',
        'for(int bh=0xa0;bh>=1;--bh)', 'for(int n=5;n<=255;++n)',
        'w={5,10,15,20}',
        '47574, 50981, 77948, 98631, 4302',
        '899246, 39064, 248070, 97042, 936',
    ]:
        assert q in pc,q

    # Input still processes at most one KEYDOWN per present; no second
    # SDL_GetTicks-based audio hold remains around the synchronous call.
    assert 'bool keyDispatchedThisFrame=false;' in main
    assert 'while(!keyDispatchedThisFrame && SDL_PollEvent(&e))' in main
    assert 'sourceSfxBusy' not in main
    assert 'commandSfxHoldStartedMs' not in main


def out_bytes(data_dir: str | Path):
    d=Path(data_dir)
    p=next((x for x in (d/'OUT.EXE',d/'out.exe') if x.exists()),None)
    assert p is not None
    b=p.read_bytes(); assert len(b)==69858
    return b