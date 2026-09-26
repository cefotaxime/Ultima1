# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M225**

이 저장소는 1987 DOS판 *Ultima I*의 실행파일/데이터 동작을 oracle로 삼아 SDL2로 재구성하고 한글화하는 프로젝트입니다. 원본 게임 데이터와 폰트 바이너리는 저장소에 포함하지 않습니다.

## M225

- 사용자가 실제 사용하는 DOSBox Staging 0.82.2 녹음을 capture oracle로 채택했습니다.
- PC-speaker/filter의 가청 PCM tail과 원본 동기식 `sound()` blocking 시간을 분리했습니다.
- projectile 첫 칸과 hit flash의 hold는 WAV 전체 길이가 아니라 `logicalDurationMs()`를 사용합니다.
- Frigate와 Aircar의 Fire는 원본 OUT.EXE대로 둘 다 **sound(8)** 입니다. M224의 임시 `VehicleFire = 6` 매핑은 폐기했습니다.
- sound(2)=hit/trap, sound(5)=success, sound(6)=failure/no-effect, sound(9)=SPACE.EXE flight pulse까지 capture oracle에 반영했습니다.
- M118 및 M212~M224의 구식 SFX audit를 현재 `SfxEvent`/`logicalDurationMs()` 구조에 맞춰 forward-compatible하게 갱신했습니다.
- M224의 RenderPresent 직전 queue 및 direct movement/collision immediate 재생 정책은 유지합니다.

## WSL 실행

```bash
cd ultima1_kr_clean_m225

./build_and_run_wsl.sh \
  /mnt/c/Users/hojun/Downloads/ultima1.zip \
  /mnt/c/Users/hojun/Downloads/H12.FNT \
  /mnt/c/Users/hojun/Downloads/ROBO.FNT
```

빌드 스크립트는 historical audit와 M225 capture-oracle audit를 먼저 수행한 뒤 SDL2 엔진을 빌드하고 실행합니다.

세부 근거는 `ORACLE_NOTES_M225.md`를 참고하십시오.
