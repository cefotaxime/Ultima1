# Ultima I 한글화 clean-room — M257

M257은 타임머신 첫 안내 팝업의 세 부분을 수정한다.

마지막 3행은 `첫째는 '발사'라고 적힌 버튼이 있다는 것,` / `둘째는 당신이 기체 안에 갇혔기 때문에` / `그 버튼을 누를 수밖에 없다는 것입니다!`로 변경하고 각각 중앙 정렬한다.

원본 DOS는 `OUT.EXE`에서 `MONDAIN.EXE`로 넘어갈 때 기존 화면을 지우지 않고, `MONDAIN.EXE`가 팝업 영역만 검게 지운다. 따라서 M257은 팝업 대기 중 직전 오버월드 화면을 고정된 배경으로 유지하고 그 위에 M256 팝업을 겹쳐 그린다.

RGBW 아래 보석은 호스트 폰트의 좁은 제어문자가 아니라 원본 BIOS teletype character `0x04`를 8x8 source cell 그대로 복원해 2배 표시한다. 각 보석은 16x16 표시 셀을 유지하며 원본 column 12/17/22/27과 EGA 색 12/10/9/15를 그대로 사용한다.

구식 audit도 갱신했다. M97의 Ready exact-source 표현식은 그대로 유지하고 M255/M256 audit을 forward-compatible하게 수정했으며 M257 전용 background/gem audit을 추가했다.
