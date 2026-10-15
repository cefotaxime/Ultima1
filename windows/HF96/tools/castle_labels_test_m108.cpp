#include "CastleLabelOverlay.h"

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

[[noreturn]] static void fail(const std::string& s){
    std::cerr<<"M108 castle labels FAIL: "<<s<<"\n";
    std::exit(1);
}

int main(){
    if(CASTLE_ZONE_LABELS.size()!=10) fail("label count");

    struct Expected{
        int map,x0,x1,y,textWidth,drawX;
        const char* original;
        const char* korean;
    };
    const std::vector<Expected> expected={
        {0,17,22, 0,48,312,"ARMOUR","방어구"},
        {0,25,28, 0,32,432,"FOOD","식량"},
        {0,16,23,17,32,320,"WEAPONRY","무기"},
        {0,28,33,17,32,496,"PRISON","감옥"},

        {1, 6,14, 0,32,168,"MESS HALL","식당"},
        {1,31,36, 0,32,544,"PRISON","감옥"},
        {1, 7,13,17,48,160,"STABLES","마구간"},
        {1,27,27,17,32,440,"F","식량"},
        {1,31,31,17,32,504,"W","무기"},
        {1,35,35,17,48,560,"A","방어구"},
    };

    for(std::size_t i=0;i<expected.size();++i){
        const auto& got=CASTLE_ZONE_LABELS[i];
        const auto& e=expected[i];
        if(got.mapIndex!=e.map || got.x0!=e.x0 || got.x1!=e.x1 || got.y!=e.y)
            fail("source span mismatch at "+std::to_string(i));
        if(std::string(got.original)!=e.original || std::string(got.korean)!=e.korean)
            fail("text mismatch at "+std::to_string(i));
        if(castleZoneLabelCenteredX(got,e.textWidth)!=e.drawX)
            fail("center mismatch at "+std::to_string(i));

        // The center of the Korean text must exactly equal the center of the
        // original A..Z tile span, including the single-letter F/W/A labels.
        const int sourceCenter2=2*16 + (got.x0+got.x1+1)*16;
        const int koreanCenter2=2*e.drawX+e.textWidth;
        if(sourceCenter2!=koreanCenter2)
            fail("center equality failed at "+std::to_string(i));
    }

    if(!castleZoneLabelCell(0,17,0) || !castleZoneLabelCell(0,22,0))
        fail("ARMOUR erase span");
    if(castleZoneLabelCell(0,23,0))
        fail("erase span leaked beyond ARMOUR");
    if(!castleZoneLabelCell(1,27,17) ||
       !castleZoneLabelCell(1,31,17) ||
       !castleZoneLabelCell(1,35,17))
        fail("F/W/A erase cells");

    std::cout<<"M108 castle Korean label overlay test OK\n";
    return 0;
}