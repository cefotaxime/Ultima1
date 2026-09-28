# Ultima I Korean Localization — M250 SPACE command localization / Sector Scan reveal

M250 applies the user-approved Korean labels for the full SPACE command set and restores the initial Sector Scan drawing order that SDL frame buffering previously hid.

SPACE command rows now use the approved Korean labels: 공격하기?, 타기?, 시전하기?, 버리기?, 진입하기?, 발포하기?, 가지기?, 하이퍼점프, 탐지하기, 뭐라고요?, 오르기?, 효과음 켜기/끄기, 열기?, 게임 종료하기?, 장비하기?, 훔치기?, 거래하기?, 자물쇠 열기?, 시점 바꾸기, 내리기?, 상태창 보기.

The original SPACE.EXE scan routine at file 0x175E..0x1787 uses X as the outer loop and Y as the inner loop, so the 7x7 grid is drawn one complete column at a time from left to right. M250 exposes that recovered draw order across SDL presentation quanta, then retains the existing 7-BIOS-tick current-sector blink.

M246 WASAPI/PCM transport remains unchanged. The full runnable source/package remains the canonical ZIP artifact; this GitHub repository is still the slim subset.
