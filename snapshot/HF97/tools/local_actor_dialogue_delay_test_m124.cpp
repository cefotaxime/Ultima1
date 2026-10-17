#include "LocalSimulation.h"
#include "OriginalData.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M124 local actor dialogue delay FAIL: "<<s<<"\n";
    std::exit(1);
}

LocalState baseLocal(bool town){
    LocalState local;
    local.active=true;
    local.inTown=town;
    local.alert=false;
    local.tilesInitialized=true;
    local.tiles.fill(1);
    local.x=30;
    local.y=10;
    local.actors[0].tile=0x13; // Bard in town / Jester in castle
    local.actors[0].x=10;
    local.actors[0].y=10;
    local.actors[0].value=1;
    return local;
}

bool contains(const std::vector<std::string>& v,const std::string& s){
    return std::find(v.begin(),v.end(),s)!=v.end();
}
}

int main(){
    OriginalData original;
    LocalSimulation sim(original);
    PlayerSave p;

    // Same deterministic branch as the historical M87 smoke:
    // randomStep X / randomStep Y / song roll => third roll < 15.
    {
        auto local=baseLocal(false);
        sim.seed(15230);
        const auto immediate=sim.updateAfterCommand(local,p);
        if(contains(immediate,"광대 그웨노가 노래합니다: 내가 열쇠를 가졌네!"))
            fail("castle Jester song leaked into immediate output");

        const auto delayed=sim.takeDelayedActorDialogue();
        if(delayed.size()!=1 ||
           delayed[0]!="광대 그웨노가 노래합니다: 내가 열쇠를 가졌네!")
            fail("castle Jester delayed row");

        if(!sim.takeDelayedActorDialogue().empty())
            fail("delayed dialogue drain is not destructive");
    }

    {
        auto local=baseLocal(true);
        sim.seed(15230);
        const auto immediate=sim.updateAfterCommand(local,p);
        if(contains(immediate,"음유시인 이올로가 노래합니다: 호 이요 히 험!"))
            fail("town Bard song leaked into immediate output");

        const auto delayed=sim.takeDelayedActorDialogue();
        if(delayed.size()!=1 ||
           delayed[0]!="음유시인 이올로가 노래합니다: 호 이요 히 험!")
            fail("town Bard delayed rows");
    }

    // No random-song branch must leave no stale delayed lines from the
    // preceding turn.
    {
        auto local=baseLocal(false);
        bool foundSilent=false;
        for(unsigned seed=1;seed<=512 && !foundSilent;++seed){
            sim.seed(static_cast<std::uint16_t>(seed));
            (void)sim.updateAfterCommand(local,p);
            const auto delayed=sim.takeDelayedActorDialogue();
            if(delayed.empty()) foundSilent=true;
        }
        if(!foundSilent) fail("could not exercise a silent actor turn");
    }

    std::cout<<"M124 Bard/Jester delayed-dialogue separation test OK\n";
    return 0;
}
