# M226 — Ready 가능하지만 Attack 불가능한 Weapon 슬롯

## 원본 판정

`OUT.EXE`의 일반 Attack command setup은 file `0x12C2`에서 시작한다.

1. `Attack with `를 출력한다.
2. `DS:2804[readyWeapon]`의 attack display name을 출력한다.
3. `DS:26DE[readyWeapon]`의 range를 검사한다.
4. range가 0이면 `?`를 출력하고 `sound(1)`을 호출한 뒤 command routine을 종료한다.
5. range가 0이 아니면 dungeon 여부를 검사하고, dungeon이 아니면 `: ` 뒤 방향 입력으로 진행한다.

range-0 branch의 exact bytes는 file `0x12F0`부터 다음과 같다.

```text
83 BC DE 26 00       cmp word [si+26DE],0
75 14                jne valid_attack
B8 35 02 50 ...      print DS:0235 = "?"
B8 01 00 50          push 1
E8 6F F3             call file 0x0676 generic sound wrapper
...
EB 25                skip valid-attack path
```

## 해당 Weapon 슬롯

원본 `DS:26DE` range table:

```text
index:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
range:  1 1 1 1 0 1 1 3 0 0  0  1  3  1  3  3
```

따라서 일반 Attack에 사용할 수 없는 Ready-able Weapon 슬롯은 정확히 네 개다.

- 4: Rope & Spikes → `밧줄과 못`
- 8: Amulet → `수호부`
- 9: Wand → `지팡이`
- 10: Staff → `마법봉`

Triangle(index 11)은 원본 range가 1이므로 정상 Attack 가능 항목이다.

## 한글 출력

range-0 슬롯에서 A를 누르면 `>[무기]@으로 공격하기?`를 출력한다. 예: `>수호부로 공격하기?`.

그 뒤 `sound(1)` BEEP을 원본 동기식 logical duration만큼 유지한 다음 command가 끝난다. M225의 capture-oracle PCM tail은 gameplay hold와 별도로 재생된다.

원본 range 검사는 dungeon/local/world 분기보다 먼저 있으므로 M226도 던전까지 같은 규칙을 적용한다.
