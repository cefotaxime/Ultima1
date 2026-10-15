#include "LocalSimulation.h"
#include "OriginalData.h"
#include <algorithm>
#include <iostream>
#include <string>

int main(){
    OriginalData original;
    LocalSimulation sim(original);

    // Seed chosen only to make the oracle branch deterministic in this smoke:
    // OUT.EXE-compatible RNG calls become 13,171,2 for
    // randomStep X / randomStep Y / song roll, so the song roll is <15.
    sim.seed(15230);

    LocalState local;
    local.active=true;
    local.inTown=false;
    local.alert=false;
    local.tilesInitialized=true;
    local.tiles.fill(1); // passable local floor; no OriginalData lookup needed
    local.x=30;
    local.y=10;
    local.actors[0].tile=0x13;
    local.actors[0].x=10;
    local.actors[0].y=10;
    local.actors[0].value=1;

    PlayerSave p;
    const auto out=sim.updateAfterCommand(local,p);
    const auto delayed=sim.takeDelayedActorDialogue();

    // M124 intentionally changes presentation only: the same RNG branch still
    // fires during updateAfterCommand(), but its ambient song rows are drained
    // separately so the SDL host can wait 500 ms before showing them.
    if(!out.empty()){
        std::cerr<<"M87/M124 jester song leaked into immediate output\n";
        return 1;
    }

    const bool line=std::find(delayed.begin(),delayed.end(),
        "광대 그웨노가 노래합니다: 내가 열쇠를 가졌네!")!=delayed.end();
    if(!line){
        std::cerr<<"M87 jester song smoke failed\n";
        for(const auto& s:delayed) std::cerr<<"  "<<s<<"\n";
        return 1;
    }
    std::cout<<"M87 jester song smoke OK\n";
    return 0;
}