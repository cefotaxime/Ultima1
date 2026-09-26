# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M226**

이 저장소는 1987 DOS판 *Ultima I*의 실행파일/데이터 동작을 oracle로 삼아 SDL2로 재구성하고 한글화하는 프로젝트입니다. 원본 게임 데이터와 폰트 바이너리는 저장소에 포함하지 않습니다.

## M226

- 원본 OUT.EXE의 normal Attack setup은 Ready weapon의 `DS:26DE` range를 먼저 검사합니다.
- range가 0인 슬롯은 정확히 **Rope & Spikes, Amulet, Wand, Staff** (index 4, 8, 9, 10)입니다.
- 현재 한글 명칭은 **밧줄과 못, 수호부, 지팡이, 마법봉**입니다.
- 이 네 항목을 Ready한 상태에서 Attack하면 `[무기]@으로 공격하기?`를 출력합니다.
- 원본의 range-0 branch는 `?` 직후 **sound(1)** BEEP을 호출하고 정상 공격 경로로 들어가지 않습니다.
- 이 검사는 dungeon/local/world 분기보다 먼저 있으므로 M226도 던전까지 동일한 동작을 적용합니다.
- M225의 DOSBox Staging 0.82.2 capture-oracle PCM과 logical sound-duration 분리는 유지합니다.
- M95/M109/M110/M111/M122/M171/M218/M225 구식 audit를 현재 구조에 맞춰 갱신했고 M226 exact-byte audit를 추가했습니다.

## WSL 실행

```bash
cd ultima1_kr_clean_m226

./build_and_run_wsl.sh \
  /mnt/c/Users/hojun/Downloads/ultima1.zip \
  /mnt/c/Users/hojun/Downloads/H12.FNT \
  /mnt/c/Users/hojun/Downloads/ROBO.FNT
```

세부 근거는 `ORACLE_NOTES_M226.md`를 참고하십시오.
