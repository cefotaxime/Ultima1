# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M224**

이 저장소는 1987 DOS판 *Ultima I*의 실행파일/데이터 동작을 oracle로 삼아 SDL2로 재구성하고 한글화하는 프로젝트입니다. 원본 게임 데이터와 폰트 바이너리는 저장소에 포함하지 않습니다.

## M224

- 적 원거리 공격: 첫 붉은 원 뒤에 임의 gameplay delay를 추가하지 않습니다.
- M212 PCM을 검사한 결과 현재 연결된 효과음은 첫 sample부터 non-zero입니다. 원거리 공격 시작 지연은 WAV 앞부분 무음 때문이 아닙니다.
- SDL/WSLg의 출력 버퍼링이 화면 표시 뒤에 추가되지 않도록 projectile 시작음을 RenderPresent 직전에 queue합니다.
- 걷기/벽 충돌 효과는 오래된 async PCM queue 뒤에 밀리지 않도록 direct command SFX를 immediate 재생합니다.
- Frigate/Aircar Fire에 별도 VehicleFire 효과음을 연결합니다. 일반 무기 Attack 효과와 분리합니다.
- 원본 delay(2)가 있는 후속 projectile cadence는 유지하며, PCM pitch/길이는 이번 단계에서 변경하지 않습니다.
- M212~M222 구식 audit를 현재 SfxEvent/latency 구조에 맞춰 갱신했습니다.

## WSL 실행

```bash
cd ultima1_kr_clean_m224

./build_and_run_wsl.sh \
  /mnt/c/Users/hojun/Downloads/ultima1.zip \
  /mnt/c/Users/hojun/Downloads/H12.FNT \
  /mnt/c/Users/hojun/Downloads/ROBO.FNT
```

빌드 스크립트가 원본 데이터 audit를 먼저 수행한 뒤 SDL2 엔진을 빌드하고 실행합니다.

세부 변경 근거는 `ORACLE_NOTES_M224.md`, 효과음 검증 순서는 `SFX_VERIFICATION_M222.md`를 참고하십시오.
