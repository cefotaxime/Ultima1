#include "OriginalData.h"
#include "LocalSimulation.h"
#include "OverworldSimulation.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

[[noreturn]] static void fail(const std::string& s){
    std::cerr << "M140 enemy projectile direction FAIL: " << s << "\n";
    std::exit(1);
}

static PlayerSave makePlayer(int px,int py,int mx,int my,int tile=23){
    PlayerSave p;
    p.x=static_cast<std::uint16_t>(px);
    p.y=static_cast<std::uint16_t>(py);
    p.hits=999;
    p.stamina=0;
    p.readyArmour=0;
    p.worldObjects[0].w[0]=10;
    p.worldObjects[0].w[2]=p.x;
    p.worldObjects[0].w[3]=p.y;
    p.worldObjects[1].w[0]=static_cast<std::uint16_t>(tile); // range-3 ranged enemy
    p.worldObjects[1].w[2]=static_cast<std::uint16_t>(mx);
    p.worldObjects[1].w[3]=static_cast<std::uint16_t>(my);
    p.worldObjects[1].w[4]=100;
    return p;
}

int main(int argc,char** argv){
    if(argc!=2) fail("usage: test GAMEDATA");
    OriginalData original;
    std::string error;
    if(!original.loadGameplayOracle(argv[1],error)) fail("oracle load: "+error);
    LocalSimulation rng(original);
    OverworldSimulation world(original,rng);

    // Find a clear 3-step southeast ray. Put the player at (+3,+1): inside
    // range 3, but not on any of the 8 legal firing lines. The attack must be
    // initiated, travel southeast without bending, and miss geometrically.
    int mx=-1,my=-1;
    for(int y=4;y<OriginalData::MAP_H-4 && mx<0;++y){
        for(int x=4;x<OriginalData::MAP_W-4;++x){
            bool clear=true;
            for(int k=1;k<=3;++k) if(original.terrainAt(x+k,y+k)==3) clear=false;
            if(clear && original.terrainAt(x+3,y+1)!=3){ mx=x; my=y; break; }
        }
    }
    if(mx<0) fail("could not locate clear off-axis test cells");
    {
        auto p=makePlayer(mx+3,my+1,mx,my);
        rng.seed(1);
        const auto messages=world.updateAfterCommand(p);
        const auto events=world.takeEnemyAttackEvents();
        if(events.size()!=1) fail("off-axis player did not trigger ranged attack");
        const std::vector<std::pair<int,int>> expected={{mx+1,my+1},{mx+2,my+2},{mx+3,my+3}};
        if(events[0].path!=expected) fail("off-axis projectile bent toward player");
        if(events[0].hit) fail("off-axis projectile hit despite missing 8-direction line");
        if(events[0].resultSuffix!="빗맞음!") fail("off-axis result was not miss");
        if(messages.size()!=1) fail("off-axis attack message missing");
    }

    // On the exact diagonal line, the same fixed heading reaches the player.
    // Geometry must terminate at the player cell rather than continue past it.
    {
        auto p=makePlayer(mx+3,my+3,mx,my);
        rng.seed(1);
        const auto messages=world.updateAfterCommand(p);
        const auto events=world.takeEnemyAttackEvents();
        if(events.size()!=1) fail("aligned diagonal attack event missing");
        const std::vector<std::pair<int,int>> expected={{mx+1,my+1},{mx+2,my+2},{mx+3,my+3}};
        if(events[0].path!=expected) fail("aligned diagonal path mismatch");
        if(messages.size()!=1) fail("aligned diagonal message missing");
    }

    std::cout << "M140 fixed 8-direction enemy projectile test OK\n";
    return 0;
}