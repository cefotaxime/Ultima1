#include "ZtatsModel.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M104 Ztats model FAIL: "<<s<<"\n";
    std::exit(1);
}

const ZtatsItem& find(const std::vector<ZtatsItem>& v,const std::string& label){
    for(const auto& x:v) if(x.label==label) return x;
    fail("missing item: "+label);
}
}

int main(){
    PlayerSave p;
    p.hits=150;
    p.strength=25;
    p.agility=25;
    p.stamina=15;
    p.charisma=15;
    p.wisdom=15;
    p.intelligence=20;
    p.coin=123;
    p.spaceKills=4;

    p.armour[1]=1; // Leather
    p.armour[2]=1; // Chain
    p.armour[4]=1; // Vacuum
    p.armour[5]=1; // Reflect
    p.readyArmour=5;

    p.weapons[1]=2; // Dagger
    p.weapons[7]=1; // Bow & Arrows -> 활과 화살
    p.readyWeapon=1;

    p.spells[1]=1; // Open -> 관 열기
    p.spells[4]=1; // Steal stays English
    p.readySpell=1;

    p.transports[1]=1; // Horse
    p.transports[5]=1; // Aircar
    p.transport=5;

    p.gems[0]=1;

    const auto v=buildZtatsItems(p);
    if(v.size()<7) fail("base stat rows missing");

    static const char* STATS[7]={
        "생명력","힘","민첩","지구력","매력","지혜","지능"};
    for(int i=0;i<7;++i){
        if(v[static_cast<std::size_t>(i)].label!=STATS[i])
            fail("base-stat order mismatch");
    }

    // Exact decimal money decomposition recovered from OUT.EXE C309..C3B5.
    if(find(v,"동화").value!=3) fail("copper pence value");
    if(find(v,"은화").value!=2) fail("silver pieces value");
    if(find(v,"금화").value!=1) fail("gold crowns value");
    if(find(v,"적함 격추").value!=4) fail("enemy vessels value");

    // Readied armour/weapon/spell/transport are red in the renderer model.
    if(!find(v,"반사복").equipped) fail("ready armour flag");
    if(!find(v,"단검").equipped) fail("ready weapon flag");
    if(!find(v,"관 열기").equipped) fail("ready spell flag");
    if(!find(v,"비행선").equipped) fail("current transport flag");

    // User-approved visible names must flow into Inventory.
    if(find(v,"활과 화살").value!=1) fail("Bow & Arrows translation");
    if(find(v,"훔치기").value!=1) fail("unused Steal spell name");
    if(find(v,"붉은색 보석").value!=1) fail("gem row");

    // Index 0 pseudo-items (Skin/Hands/Prayer/Foot) are not shown by C3E4..C46E.
    for(const auto& x:v){
        if(x.label=="맨몸" || x.label=="맨손" || x.label=="기도" || x.label=="Foot")
            fail("index-0 pseudo-item leaked into Inventory");
    }


    // M106 localized leader geometry: no blank gap before the number, and
    // every number has the same 240-pixel right edge within its column.
    if(ZTATS_VALUE_RIGHT_OFFSET!=240) fail("M106 value right edge offset");

    // "생명력" = 3 Hangul glyphs = 48 px; "150" = 24 px; "." = 8 px.
    {
        const int labelW=48, numberW=24, dotW=8;
        const int numberX=ztatsNumberOffset(numberW);
        const int dots=ztatsLeaderDots(labelW,numberW,dotW);
        if(labelW+dots*dotW!=numberX)
            fail("three-digit row has a dot/number gap");
        if(numberX+numberW!=ZTATS_VALUE_RIGHT_OFFSET)
            fail("three-digit right edge is not aligned");
        if(dots!=21)
            fail("unexpected three-digit leader length");
    }

    // "가죽 갑옷" = 72 px; one-digit value = 8 px.  The last dot must
    // likewise touch the digit exactly.
    {
        const int labelW=72, numberW=8, dotW=8;
        const int numberX=ztatsNumberOffset(numberW);
        const int dots=ztatsLeaderDots(labelW,numberW,dotW);
        if(labelW+dots*dotW!=numberX)
            fail("one-digit row has a dot/number gap");
        if(numberX+numberW!=ZTATS_VALUE_RIGHT_OFFSET)
            fail("one-digit right edge is not aligned");
    }

    PlayerSave full=p;
    full.armour.fill(1);
    full.weapons.fill(1);
    full.spells.fill(1);
    full.transports.fill(1);
    full.gems.fill(1);
    full.coin=999;
    full.spaceKills=20;
    if(!ztatsHasMore(full,0)) fail("More page not detected");

    std::cout<<"M106 Ztats dense leader/two-column model test OK\n";
    return 0;
}
