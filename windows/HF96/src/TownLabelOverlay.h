#pragma once

#include <array>

struct TownWallLabel {
    int mapIndex;
    int x0;
    int y0;
    int x1;
    int y1;
    const char* original;
    const char* korean;
};

// Exact shop-sign character-tile spans recovered from the user's original
// TCD.BIN.  A..Z are EGATOWN tiles 24..49 and tile 23 is the blank inverse
// sign block used between words such as "THE  PUB".  We erase the whole
// original sign span to tile 23, then redraw the approved Korean label in
// black on the same white block.  Spaced English words become one Korean word.
inline constexpr std::array<TownWallLabel,29> TOWN_WALL_LABELS{{
    {2,21, 0,25, 0,"MAGIC",    "마법"},
    {2, 3, 2,11, 2,"TRANSPORT","운송"},
    {2,31, 2,33, 2,"PUB",      "주점"},
    {2, 3,15, 8,15,"ARMOUR",   "방어구"},
    {2,11,15,14,15,"WEAP",     "무기"},
    {2,22,15,25,15,"FOOD",     "식량"},

    {3,27, 0,34, 0,"THE  INN", "여관"},
    {3, 3, 2, 8, 2,"ARMOUR",   "방어구"},
    {3,10, 2,13, 2,"ARMS",     "방어구"},
    {3,26,15,32,15,"GROCERY",  "식료품"},
    {3, 2,17, 6,17,"MAGIC",    "마법"},

    {4,15, 0,22, 0,"THE  PUB", "주점"},
    {4,32, 0,36, 0,"MAGIC",    "마법"},
    {4, 4, 2, 9, 2,"ARMOUR",   "방어구"},
    {4, 3,15, 6,15,"ARMS",     "방어구"},

    {5, 1, 0, 7, 0,"WEAPONS",  "무기"},
    {5, 9, 0,14, 0,"ARMOUR",   "방어구"},
    {5,16, 0,19, 0,"FOOD",     "식량"},
    {5,22, 0,24, 0,"PUB",      "주점"},
    {5,26, 0,30, 0,"TRANS",    "운송"},
    {5,32, 0,36, 0,"MAGIC",    "마법"},

    {6,10, 7,10,10,"FOOD",     "식량"},
    {6,27, 7,27,10,"ARMS",     "방어구"},

    {7,15, 1,22, 1,"THE  PUB", "주점"},
    {7, 6, 3, 9, 3,"FOOD",     "식량"},
    {7,27, 3,31, 3,"TRANS",    "운송"},
    {7, 6,14,11,14,"ARMOUR",   "방어구"},
    {7,26,14,31,14,"WEAPON",   "무기"},

    {8,26, 2,32, 2,"THE PUB",  "주점"},
}};

// Map 8/9 entries kept separate to keep the constexpr array readable.
inline constexpr std::array<TownWallLabel,6> TOWN_WALL_LABELS_EXTRA{{
    {8, 6, 3, 9, 3,"FOOD",     "식량"},
    {8,15, 3,21, 3,"WEAPONS",  "무기"},
    {9, 1, 0, 7, 0,"WEAPONS",  "무기"},
    {9,14, 0,18, 0,"TRANS",    "운송"},
    {9,21, 0,27, 0,"THE INN",  "여관"},
    {9,32, 0,36, 0,"MAGIC",    "마법"},
}};

inline constexpr std::array<TownWallLabel,2> TOWN_WALL_LABELS_EXTRA2{{
    {9, 0,11, 0,16,"ARMOUR",   "방어구"},
    {9,32,17,35,17,"FOOD",     "식량"},
}};

template<class F>
inline void forEachTownWallLabel(F&& f){
    for(const auto& v:TOWN_WALL_LABELS) f(v);
    for(const auto& v:TOWN_WALL_LABELS_EXTRA) f(v);
    for(const auto& v:TOWN_WALL_LABELS_EXTRA2) f(v);
}

inline bool townWallLabelCell(int mapIndex,int x,int y){
    bool found=false;
    forEachTownWallLabel([&](const TownWallLabel& label){
        if(label.mapIndex==mapIndex && x>=label.x0 && x<=label.x1 && y>=label.y0 && y<=label.y1)
            found=true;
    });
    return found;
}