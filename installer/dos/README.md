# DOS판 한국어 설치기

배포 파일 (로컬 전용): `C:\Users\hojun\Downloads\Ultima1K_DOS_Installer\Ultima1K(DOS)_Setup.exe`

기존 Windows판 설치기의 C# 단일 EXE + 내장 PowerShell WinForms GUI + payload.zip 구조를 계승한다.

- 선택한 설치 기본 경로 아래에 `Ultima1K(DOS)` 생성
- 원본 `ULTIMA.EXE` 등 9개 필수 파일 검증 후 설치, 원본 폴더는 변경하지 않음
- 원본 게임 데이터는 설치 파일에 내장하지 않고 사용자가 선택한 원본 경로에서 필요한 25개를 `GAMEDATA`로 복사
- `SAVEGAME`은 생성만 하고 기존 저장파일 덮어쓰기/삭제 금지
- `Ultima1K.bat`으로 동봉 `DOSBox/DOSBox.exe`를 시작; DOSBox 0.74-3 포터블, 기본 전체화면
- `dosbox_ultima1.conf`의 `[autoexec]`에서 한국어판 폴더를 마운트하고 `ULTIHF99.EXE` 실행
- DOSBox 저작권 고지 `COPYING.txt`와 AUTHORS 및 매뉴얼 포함. 공개 배포 전 GPL 관련 소스 코드 제공 의무 및 SDL 라이선스 검토 필요

빌드: Windows PowerShell에서 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File installer\dos\build_installer.ps1`. 스크립트는 로컬 개발기 설치 경로를 참조하며 EXE와 payload.zip은 저작권 관련 파일을 포함할 수 있어 **Git에 업로드하지 않음**.

2026-10-17 검증: standalone 컴파일 완료, 압축 해제 페이로드에 게임 EXE, DOSBox, 한글 폰트, 전체화면 설정 확인. 독립 임시 설치 디렉터리에서 Ultima1K.bat 실행 후 포터블 DOSBox 프로세스 시작 확인. 실제 GUI 설치·종료 과정은 사용자 확인 전.
