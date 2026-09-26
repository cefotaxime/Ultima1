# M223 — M118 효과음 audit 호환성 수정

M223은 런타임 동작 변경이 아니다. M222에서 효과음 호출을 `SfxEvent`로 중앙화한 뒤, 역사적 `tools/audit_m118_enemy_projectile.py`가 여전히 `sfx.durationMs(3)`, `playImmediate(2)`, `sfx.durationMs(2)`라는 구식 숫자형 source 문자열을 요구해 빌드 스크립트가 중단되던 문제를 수정한다.

## 변경
- `sfx.durationMs(3)` 기대값 → `sfx.durationMs(SfxEvent::EnemyRangedLaunch)`
- `sfx.playImmediate(2)` 기대값 → `sfx.playImmediate(SfxEvent::CombatHit)`
- `sfx.durationMs(2)` 기대값 → `sfx.durationMs(SfxEvent::CombatHit)`

원본 `OUT.EXE`의 M118 byte-oracle 검사는 그대로 유지한다. 게임의 projectile draw/sound/delay ordering 및 실제 효과음 매핑은 M222와 동일하다. 피치/절대 속도 보정도 하지 않는다.
