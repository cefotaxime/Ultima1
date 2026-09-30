# Ultima I 한글화 clean-room — M260 hotfix1

M260은 몬데인 최종전의 전투 문구와 승리/패배 연출을 원본 방식에 맞춰 복원한다.

hotfix1은 구식 M164 audit의 회귀 오류를 수정한다. 몬데인 전투의 `Missed!`는 승인된 번역인 `빗맞음!`을 사용하고, 일반 로컬 전투의 `공격이 빗나갑니다!`는 그대로 유지한다. 이번 hotfix는 게임 코드를 변경하지 않고 audit만 forward-compatible하게 수정한다.
