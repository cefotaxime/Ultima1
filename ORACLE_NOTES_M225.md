# M225 — DOSBox Staging capture-oracle SFX

## 기준

사용자가 실제로 사용하는 DOSBox Staging 0.82.2 설정에서 직접 WAV를 녹음했다.

- `core = auto` (Ultima I real mode에서는 normal core)
- `cpu_cycles = 3000`
- `pcspeaker = impulse`
- `pcspeaker_filter = on`
- mixer rate 48 kHz

M212의 synthetic PCM은 효과음의 구조를 찾는 데 유용했지만 실제 사용자 환경보다 시간축이 느렸다. M225에서는 synthetic PCM을 런타임 기준으로 사용하지 않는다.

## capture-oracle mapping

| sound ID | 이벤트 | 직접 녹음 |
|---:|---|---|
| 0 | outdoor terrain/transport collision | `blocked_woods.wav` |
| 1 | local blocked / invalid X-it | `Xit_unavailable.wav` |
| 2 | combat hit / trap | `set_off_a_trap.wav` |
| 3 | enemy ranged launch | `enemy attack.wav` |
| 4 | movement / footstep | `footstep_field.wav` |
| 5 | positive/success | `success.wav` |
| 6 | failure/no-effect | `failure.wav` |
| 7 | normal Attack launch | `blaster.wav` |
| 8 | Frigate/Aircar Fire | `airship_fire.wav` |
| 9 | SPACE.EXE flight pulse | `space.wav` |

ID 8은 OUT.EXE Fire 루틴에서 Frigate와 Aircar 모두 동일하게 사용한다. M224의 임시 `VehicleFire = 6`은 잘못된 매핑이므로 M225에서 `VehicleFire = 8`로 수정한다.

## PCM 길이와 게임 blocking 시간 분리

DOSBox Staging의 impulse speaker와 filter는 원본 `sound()` 루틴이 반환된 뒤에도 audible tail을 남긴다. 따라서 WAV 전체 길이를 projectile hold 시간으로 사용하면 원본보다 느려진다.

M225는 presentation PCM과 원본 synchronous blocking duration을 분리한다.

`logicalDurationMs()`의 정수-ms oracle:

- ID 0: 48 ms
- ID 1: 51 ms
- ID 2: 78 ms
- ID 3: 99 ms
- ID 4: 5 ms
- ID 5: 900 ms
- ID 6: 40 ms
- ID 7: 249 ms
- ID 8: 98 ms
- ID 9: 1 ms

## 원칙

- 새 인위적 gameplay delay를 넣지 않는다.
- pitch/length를 임의 계수로 재생하지 않는다. 직접 녹음한 capture PCM을 사용한다.
- 원본 호출 ID와 gameplay timing은 byte oracle/capture oracle로 분리 검증한다.
