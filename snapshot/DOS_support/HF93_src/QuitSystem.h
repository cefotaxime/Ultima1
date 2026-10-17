#pragma once

#include <string>

// OUT.EXE 0x5D54 checks DS:8B9A.  Zero is the outdoor/world-map mode;
// any nonzero local/dungeon mode is rejected before the save routine.
inline bool quitSaveAllowedOutdoors(bool localActive,bool dungeonActive,
                                    bool spaceActive=false,bool mondainActive=false){
    return !localActive && !dungeonActive && !spaceActive && !mondainActive;
}

inline const char* quitSavingText(){
    return "게임 종료하기 - 저장중...";
}

inline const char* quitSavedSuffix(){
    return "저장 완료.";
}

inline const char* quitIndoorsText(){
    return "게임 종료하기 - 야외에서만 가능합니다!";
}

// The DOS code has no explicit timer: "saving game..." is drawn first, then the
// synchronous 820-byte file write runs, then "saved." is printed.  On modern
// storage that natural pause is effectively invisible, so M107 emulates the
// period-visible gap requested by the user.
constexpr unsigned QUIT_SAVE_VISIBLE_DELAY_MS=500;
