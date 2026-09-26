# M224 — 효과음 시작 지연 / Fire 복원

## 임의 delay 재검증

M221 이후 원거리 공격 첫 칸 앞에 별도의 host sleep/delay를 넣지 않는다. 첫 칸 이후의 projectile cadence는 원본 `delay(2)` 호출을 표현하는 기존 경로이며 첫 칸에는 적용하지 않는다.

M224에서 M212 기준 PCM을 직접 검사했다. 현재 연결된 ID 0,1,2,3,4,6,7은 첫 PCM frame부터 non-zero이므로 사용자가 들은 시작 지연은 WAV 앞쪽 무음 때문이 아니다.

M224는 SDL draw command가 모두 작성된 뒤 RenderPresent 직전에 PCM을 queue한다. host audio latency를 화면 present와 겹치며 게임 로직에 새 sleep/delay를 추가하지 않는다.

이동/충돌음은 이전 async clip 뒤에 누적되지 않도록 direct command 효과에 playImmediate를 사용한다.

## Frigate/Aircar Fire

overworld Fire는 일반 무기 Attack과 별도 효과음으로 취급한다. M224는 `SfxEvent::VehicleFire = 6`을 추가하고 Frigate/Aircar Fire의 첫 projectile cell에서 재생한다. 일반 무기 Attack의 ID 7과 분리한다.

SPACE.EXE Fire는 별도 subsystem이며 이 변경에 포함하지 않는다.

## 변경하지 않은 것

- M212 PCM의 pitch/길이/파형 자체는 변경하지 않음
- 적 원거리 공격 ID 3, 적중 ID 2 event identity 유지
- 원본 delay(2)가 존재하는 후속 projectile step 유지
- 전체 효과음 속도/피치 보정은 효과음 매핑 완료 후 일괄 처리
