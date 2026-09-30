# Ultima I 한글화 clean-room — M259 hotfix2

M259는 몬데인 최종 전투장의 slot 매핑과 타임머신 도입부 진행을 수정한다. hotfix2는 hotfix1에서 생긴 launcher 회귀 오류를 수정한다.

최종 audit 문구를 다시 구식 audit이 기대하는 숫자형 `[audit] All audits through M259 OK`로 고정하고, `M259 hotfix2` 표기는 별도 줄로 출력한다. 또한 배포 ZIP에는 `build/`와 `CMakeCache.txt`를 넣지 않아 어느 WSL 경로에 풀어도 새로 configure하도록 유지한다.
