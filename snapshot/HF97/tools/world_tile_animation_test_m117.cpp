#include "WorldTileAnimation.h"

#include <iostream>
#include <string>

[[noreturn]] static void fail(const std::string& s){
    std::cerr<<"M117 world tile animation FAIL: "<<s<<"\n";
    std::exit(1);
}

int main(){
    // Deterministic B157 pairs.
    if(worldAnimatedPhysicalTile(14,0,0)!=14) fail("frigate step0");
    if(worldAnimatedPhysicalTile(14,1,0)!=15) fail("frigate step1");
    if(worldAnimatedPhysicalTile(14,2,0)!=14) fail("frigate step2");
    if(worldAnimatedPhysicalTile(25,0,0)!=25) fail("pirate step0");
    if(worldAnimatedPhysicalTile(25,1,0)!=26) fail("pirate step1");

    // Every ordinary monster producer base 19+2*i selects base/base+1.
    for(int base=19;base<=47;base+=2){
        if(base==25) continue; // Pirate ship has deterministic parity branch first.
        if(!worldTileRandomPairBase(base))
            fail("missing random pair base "+std::to_string(base));
        if(worldAnimatedPhysicalTile(base,0,0)!=base)
            fail("random A frame "+std::to_string(base));
        if(worldAnimatedPhysicalTile(base,0,1)!=base+1)
            fail("random B frame "+std::to_string(base));
    }

    // Static transport/player tiles remain unchanged.
    for(int t: {10,11,12,13,16,17,18,49,50,51}){
        if(worldAnimatedPhysicalTile(t,1,1)!=t)
            fail("static tile changed "+std::to_string(t));
    }

    if(worldTileRandomPairBase(25))
        fail("pirate incorrectly treated as random");
    if(!worldTileDeterministicPairBase(14) ||
       !worldTileDeterministicPairBase(25))
        fail("deterministic pair classification");

    std::cout<<"M117 B157 physical-pair frame selection test OK\n";
    return 0;
}
