# Ultima I 한국어판 1.0K — 현재 개발·배포 기준

**GitHub가 유일한 개발 소스 기준입니다.** 기존 개발 PC의 백업 폴더나 임시 빌드 폴더는 필요하지 않습니다.

- **최신 게임 소스:** [snapshot/HF97](snapshot/HF97) 및 [snapshot/DOS_support](snapshot/DOS_support) (HF99 동기화 상태)
- **Windows·DOS 자동 빌드:** [GitHub Actions](https://github.com/cefotaxime/Ultima1/actions/workflows/release-build.yml)
- **영구 다운로드:** [GitHub Releases — 1.0K source build](https://github.com/cefotaxime/Ultima1/releases/tag/v1.0K-source-build)
- **재현 가능한 빌드 설명:** [ci/README.md](ci/README.md)
- **설치기 소스:** [installer/NativeInstaller.cs](installer/NativeInstaller.cs), [installer/dos/InnoSetup_DOS.iss](installer/dos/InnoSetup_DOS.iss)

GitHub Actions는 빈 빌드 환경에서 컴파일러·SDL2·DOSBox·CWSDPMI·Inno Setup을 설치하고, 두 게임 실행파일과 설치 EXE를 생성한 뒤 **각각 7z로 압축하고 무결성을 검사**합니다. DOS 설치기는 가상 원본 게임 데이터로 설치·저장파일 보존 시험도 수행합니다. 별도의 정품 Ultima I 원본 게임 파일은 사용자가 설치 시 지정해야 하며 저장소·배포파일에 포함하지 않습니다.

**주의:** GitHub에서 새로 컴파일한 배포본은 이전 PC에서 바이러스 검사를 승인받은 EXE와 해시가 다릅니다. 자동 빌드 및 설치 시험 통과와 실제 게임 전 구간 검증·보안업체 평판 승인은 별개입니다. 영구 릴리스는 이를 구분하기 위해 사전 릴리스(candidate)로 표시합니다.

---

## 이전 개발 기록 (보존)

# HF78 — Quest Status (Shift-Q)

Windows HF78 build: `Ultima1_KR_Work/packages/HF78_QUEST_STATUS_KOREAN_TITLE.zip` on the connected PC. Source changes are provided as [HF78/hf78_quest_changes.patch](HF78/hf78_quest_changes.patch) against the HF77 source; HF77 itself is based on HF76 with a Shift-Q key interception (no more legacy Q Save fallthrough).

- Shift-Q opens a modal quest journal; Space closes it.
- An empty journal displays `수락한 퀘스트가 없습니다!`.
- Accepted quests appear in receipt order with castle names, goal descriptions, aligned dot leaders, and one of `수락`, `성공`, `완료`.
- Quest journal uses `PLAYER*.U1.krquests` alongside original DOS save files and never modifies the DOS save format.
- Window title: `울티마 I 한국어판`.

**Validation:** MinGW Windows build succeeded; full interactive gameplay verification remains pending.

---

# HF76 — Command help and Korean IME support

**Build source:** Use all source and support files from `HF75/`, replacing **only** `HF75/src/main.cpp` with `HF76/src/main.cpp`.

Changes:
- Command-help popup keeps the A–Z entries and Shift-H / Shift-V entries in their previous positions.
- Two lines below the left-column K entry, place `Alt-Enter 전체 화면`, followed on the next line by `Shift-Q 퀘스트 현황`.
- Alphabetic commands use SDL **physical scancodes** instead of layout-dependent keysyms, including Shift+H, Shift+V, and other Shift+letter commands, even while Korean IME is enabled.
- **Shift-Q is listed in help but its quest-status popup is not implemented in this revision.**

## Windows build and package

HF76 compiled successfully as MinGW `u1kr_clean.exe`. The user PC has the packaged Windows distribution at `Ultima1_KR_Work/packages/HF76_HELP_HANGUL.zip`. For an independent build, copy `HF76/src/main.cpp` over the corresponding file in a local HF75 checkout, then follow the HF75 build steps below.

---

# Ultima I Korean Localization — HF75

Current Windows build source: [HF75/](HF75/). Earlier M262 content remains below as historical release notes.

## HF75 sound changes (2026-10-09)
- Dungeon: Apple II behavior. An attack when no monster is in range prints the existing no-target message **without attack-launch sound**. Attacks against an actual monster retain the existing attack/hit sound behavior.
- Space Top View: successful **Thrust** and **Retro** inputs play the **same OutdoorLandMove sound event** used for ordinary field foot movement, per project approval. Failed moves, ship-selection controls and Front View reticle controls do not trigger this sound.
- Previously implemented Front View / HyperJump audio is unchanged.

## Windows build
The main source is under `HF75/src/` with `HF75/third_party/fake6502/`. Build the `u1kr_clean` target using a C++17 toolchain and SDL2 CMake package. Example for the project's WSL MinGW setup:

```bash
cmake -S HF75 -B build/HF75_win64 \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=/usr/bin/x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/x86_64-w64-mingw32-g++ \
  -DSDL2_DIR=/path/to/SDL2/x86_64-w64-mingw32/lib/cmake/SDL2
cmake --build build/HF75_win64 --target u1kr_clean -j4
```

Package the resulting `u1kr_clean.exe` as `Ultima1KR.exe` alongside the existing licensed game data, fonts and SDL2 runtime DLLs. Original game assets are not included in the source tree.

## History

# Ultima I Korean Localization — M262

M262 restores the original light-cyan text color for the framed MONDAIN popups shown before and after the final encounter. The four Time Machine narration pages, the victory popup, and the defeat popup now use EGA light cyan. The RGBW labels on the first Time Machine page follow the same popup text color; the four BIOS gem glyphs retain their original red/green/blue/white colors.

Historical M256 was updated so it no longer pins the obsolete white popup text implementation.
