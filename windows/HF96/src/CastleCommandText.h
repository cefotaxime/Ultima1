#pragma once

#include <string>

#include "KoreanText.h"
#include "OriginalData.h"

// OUT.EXE file 0x584B (Inform and search): outside dungeon mode the original
// queries the player's saved overworld X/Y.  When that coordinate is a named
// location it prints the location name on the row after "Inform and search".
// Castles (location ids 33..40) are printed without the town prefix.
inline std::string 성_탐지_결과(const OriginalData& original,const PlayerSave& p){
    const LocationInfo* loc=original.locationAt(static_cast<int>(p.x),static_cast<int>(p.y));
    if(!loc || loc->kind!=LocationKind::Castle) return {};
    if(const char* ko=성_이름_한글(loc->name)) return ko;
    return loc->name;
}