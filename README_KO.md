# Ultima I 한글화 clean-room — M259 hotfix3

M259는 몬데인 최종 전투장의 slot 매핑과 타임머신 도입부를 수정한다. hotfix3는 배포 경로 검증 audit 자체의 오류를 수정한다.

실행기는 CMake configure 전에 기존 `build/`를 삭제한다. configure/build가 끝난 뒤에는 `CMakeCache.txt`가 존재하는 것이 정상이다. 이제 relocation audit은 이 파일이 없어야 한다고 검사하지 않고, cache의 `CMAKE_HOME_DIRECTORY`가 현재 압축을 푼 소스 경로와 일치하는지를 확인한다. 다른 경로에서 만들어진 stale cache는 실패시킨다.

최종 표식은 계속 `[audit] All audits through M259 OK`를 사용하고, 패키지 표기는 별도 줄의 `M259 hotfix3`로 출력한다.
