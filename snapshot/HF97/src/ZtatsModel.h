#pragma once

#include "KoreanText.h"
#include "OriginalData.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

struct ZtatsItem {
    std::string label;
    int value=0;
    bool equipped=false;
};


// Localized Ztats two-column leader geometry.
// From each column's text start to the *right edge* of its value, reserve 240 px.
// This extends M105 by exactly 64 px (8 ASCII dot widths) for a three-digit value.
constexpr int ZTATS_VALUE_RIGHT_OFFSET=240;

inline int ztatsNumberOffset(int numberWidth){
    return ZTATS_VALUE_RIGHT_OFFSET-numberWidth;
}

inline int ztatsLeaderDots(int labelWidth,int numberWidth,int dotWidth){
    if(dotWidth<=0) return 0;
    const int leaderWidth=ztatsNumberOffset(numberWidth)-labelWidth;
    return std::max(0,leaderWidth/dotWidth);
}

inline std::vector<ZtatsItem> buildZtatsItems(const PlayerSave& p){
    std::vector<ZtatsItem> items;

    auto add=[&](std::string label,int value,bool equipped=false){
        if(value!=0) items.push_back(ZtatsItem{std::move(label),value,equipped});
    };

    // OUT.EXE C2EB -> C13D: the seven base statistics are always present.
    items.push_back({"생명력",static_cast<int>(p.hits),false});
    items.push_back({"힘",static_cast<int>(p.strength),false});
    items.push_back({"민첩",static_cast<int>(p.agility),false});
    items.push_back({"지구력",static_cast<int>(p.stamina),false});
    items.push_back({"매력",static_cast<int>(p.charisma),false});
    items.push_back({"지혜",static_cast<int>(p.wisdom),false});
    items.push_back({"지능",static_cast<int>(p.intelligence),false});

    // OUT.EXE C309..C3B5: money is displayed as decimal denominations.
    const int coin=static_cast<int>(p.coin);
    if((coin%10)!=0) add("동화",coin%10);
    if((coin%100)>9) add("은화",(coin%100)/10);
    if(coin>99) add("금화",coin/100);

    if(p.spaceKills!=0) add("적함 격추",static_cast<int>(p.spaceKills));

    static constexpr const char* ARMOUR[6]={
        "맨몸","가죽 갑옷","사슬 갑옷","판금 갑옷","우주복","반사복"};
    static constexpr const char* SPELLS[11]={
        "기도","관 열기","상자 열기","마탄","훔치기","상승 사다리",
        "하강 사다리","순간이동","마력장 생성","마력장 파괴","죽음"};

    // OUT.EXE C3E4..C46E: inventory groups are appended in this order.
    // Index 0 is intentionally omitted for armour/weapon/spell/transport.
    for(int i=1;i<6;++i)
        add(ARMOUR[i],static_cast<int>(p.armour[static_cast<std::size_t>(i)]),
            p.readyArmour==i);
    for(int i=1;i<16;++i)
        add(무기_이름_한글(i),static_cast<int>(p.weapons[static_cast<std::size_t>(i)]),
            p.readyWeapon==i);
    for(int i=1;i<11;++i)
        add(SPELLS[i],static_cast<int>(p.spells[static_cast<std::size_t>(i)]),
            p.readySpell==i);
    for(int i=1;i<8;++i)
        add(운송수단_이름_한글(i),static_cast<int>(p.transports[static_cast<std::size_t>(i)]),
            p.transport==i);
    for(int i=0;i<4;++i)
        add(보석_이름_한글(i),static_cast<int>(p.gems[static_cast<std::size_t>(i)]),false);

    return items;
}

inline bool ztatsHasMore(const PlayerSave& p,int page){
    constexpr int PER_PAGE=26;
    return (page+1)*PER_PAGE < static_cast<int>(buildZtatsItems(p).size());
}