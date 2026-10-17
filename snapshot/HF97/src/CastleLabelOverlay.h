#pragma once

#include <array>

struct CastleZoneLabel {
    int mapIndex;
    int x0;
    int x1;
    int y;
    const char* original;
    const char* korean;
};

// TCD castle map 0 / 1 contain the visible English zone names as A..Z
// EGATOWN character tiles on rows y=0 / y=17.  This table describes the exact
// original spans so the renderer can erase only those text tiles and draw the
// approved Korean labels centered on the same source-space centers.
inline constexpr std::array<CastleZoneLabel,10> CASTLE_ZONE_LABELS{{
    {0,17,22, 0,"ARMOUR",  "방어구"},
    {0,25,28, 0,"FOOD",    "식량"},
    {0,16,23,17,"WEAPONRY","무기"},
    {0,28,33,17,"PRISON",  "감옥"},

    {1, 6,14, 0,"MESS HALL","식당"},
    {1,31,36, 0,"PRISON",   "감옥"},
    {1, 7,13,17,"STABLES",  "마구간"},
    {1,27,27,17,"F",        "식량"},
    {1,31,31,17,"W",        "무기"},
    {1,35,35,17,"A",        "방어구"},
}};

inline bool castleZoneLabelCell(int mapIndex,int x,int y){
    for(const auto& label:CASTLE_ZONE_LABELS){
        if(label.mapIndex==mapIndex && label.y==y && x>=label.x0 && x<=label.x1)
            return true;
    }
    return false;
}


inline int castleZoneLabelCenteredX(const CastleZoneLabel& label,int textWidth,
                                    int viewX=16,int tileWidth=16){
    const int originalX=viewX+label.x0*tileWidth;
    const int originalW=(label.x1-label.x0+1)*tileWidth;
    return originalX+(originalW-textWidth)/2;
}