# Ultima I Korean Localization (clean-room SDL2)

현재 작업 기준: **M223**

이 저장소는 1987 DOS판 *Ultima I*의 실행파일/데이터 동작을 oracle로 삼아 SDL2로 재구성하고 한글화하는 프로젝트입니다. 원본 게임 데이터와 폰트 바이너리는 저장소에 포함하지 않습니다.

## 현재 패키지

`ultima1_kr_clean_m223.zip`

M223은 M222의 런타임 효과음 동작을 바꾸지 않고, `SfxEvent` 열거형 전환 뒤 남아 있던 역사적 M118 audit의 구식 숫자 호출 검사를 수정합니다.

- `sound(3)` → `SfxEvent::EnemyRangedLaunch`
- `sound(2)` → `SfxEvent::CombatHit`
- 원본 OUT.EXE byte oracle은 변경하지 않음
- 피치/절대 속도 조정은 전체 효과음 매핑 완료 후 일괄 수행

## WSL 실행

사용자가 소유한 다음 파일이 필요합니다.

```text
ultima1.zip
H12.FNT
ROBO.FNT
```

패키지를 압축 해제한 뒤:

```bash
cd ultima1_kr_clean_m223

./build_and_run_wsl.sh \
  /mnt/c/Users/hojun/Downloads/ultima1.zip \
  /mnt/c/Users/hojun/Downloads/H12.FNT \
  /mnt/c/Users/hojun/Downloads/ROBO.FNT
```

스크립트는 원본 데이터 기반 audit를 순서대로 실행한 뒤 SDL2 빌드와 회귀 검증을 수행합니다.

## 검증

M223에서 확인한 항목:

- M118 enemy-projectile source/oracle compatibility PASS
- M222 centralized SFX oracle policy PASS
- M223 historical M118 SFX audit compatibility PASS
- M128 이후 주요 source-only historical audits PASS

세부 변경 근거는 패키지의 `ORACLE_NOTES_M223.md`, 효과음 검증 절차는 `SFX_VERIFICATION_M222.md`를 참고하십시오.
