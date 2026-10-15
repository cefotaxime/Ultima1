#include "OriginalData.h"
#include "LocalSimulation.h"
#include "OverworldSimulation.h"

#include <iostream>
#include <string>
#include <vector>

[[noreturn]] static void fail(const std::string& s){
    std::cerr<<"M118 enemy projectile FAIL: "<<s<<"\n";
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

    p.worldObjects[1].w[0]=static_cast<std::uint16_t>(tile); // Dragon turtle by default, range 3
    p.worldObjects[1].w[2]=static_cast<std::uint16_t>(mx);
    p.worldObjects[1].w[3]=static_cast<std::uint16_t>(my);
    p.worldObjects[1].w[4]=100;
    return p;
}

int main(int argc,char** argv){
    if(argc!=2) fail("usage: test GAMEDATA");

    OriginalData original;
    std::string error;
    if(!original.loadGameplayOracle(argv[1],error))
        fail("could not load gameplay oracle: "+error);

    LocalSimulation rng(original);
    OverworldSimulation world(original,rng);

    // MAP.BIN row y=8 has non-mountain cells x=14..18. Put the monster at 15
    // and player at 18: OUT.EXE 0x83FC must draw tile50 at 16,17,18 in order.
    {
        auto p=makePlayer(18,8,15,8);
        rng.seed(1);
        const auto messages=world.updateAfterCommand(p);
        const auto events=world.takeEnemyAttackEvents();

        if(events.size()!=1) fail("expected one enemy attack event");
        const auto& ev=events[0];
        const std::vector<std::pair<int,int>> expected={{16,8},{17,8},{18,8}};
        if(ev.path!=expected) fail("three-cell projectile path mismatch");
        if(ev.attackPrefix!="거북용의 공격! ")
            fail("attack prefix mismatch");
        if(messages.size()!=1 || messages[0]!=ev.attackPrefix+ev.resultSuffix)
            fail("logical message/event mismatch");
        if(!world.takeEnemyAttackEvents().empty())
            fail("takeEnemyAttackEvents did not drain");
    }

    // The terminal mountain cell is visible: 128,10 -> mountain 129,10 ->
    // player 130,10. The original blits tile50 first, then tests terrain==3.
    {
        auto p=makePlayer(130,10,128,10);
        rng.seed(7);
        const auto messages=world.updateAfterCommand(p);
        const auto events=world.takeEnemyAttackEvents();
        if(events.size()!=1) fail("mountain event missing");
        if(events[0].path.size()!=1 || events[0].path[0]!=std::pair<int,int>{129,10})
            fail("mountain terminal cell was not retained in visual path");
        if(events[0].hit) fail("mountain-blocked projectile cannot hit");
        if(events[0].resultSuffix!="빗맞음!") fail("mountain miss suffix");
        if(messages.size()!=1) fail("mountain miss message count");
    }

    // Exercise both the successful-hit and hit-roll-miss branches while the
    // physical path remains identical.
    bool sawHit=false,sawMiss=false;
    for(unsigned seed=1;seed<=1024 && !(sawHit&&sawMiss);++seed){
        auto p=makePlayer(18,8,15,8);
        const auto before=p.hits;
        rng.seed(static_cast<std::uint16_t>(seed));
        const auto messages=world.updateAfterCommand(p);
        const auto events=world.takeEnemyAttackEvents();
        if(events.size()!=1) fail("branch event count");
        const auto& ev=events[0];

        if(ev.hit){
            if(ev.resultSuffix.find("적중! ")!=0 ||
               ev.resultSuffix.find("의 피해량!")==std::string::npos)
                fail("hit suffix");
            if(p.hits>=before) fail("successful hit did not reduce HP");
            sawHit=true;
        }else{
            if(ev.resultSuffix!="빗맞음!") fail("miss suffix");
            if(p.hits!=before) fail("miss changed HP");
            sawMiss=true;
        }

        if(messages.size()!=1 || messages[0]!=ev.attackPrefix+ev.resultSuffix)
            fail("branch message mismatch");
    }
    if(!sawHit || !sawMiss) fail("could not exercise both hit and miss");

    std::cout<<"M118 OUT.EXE 0x83FC enemy projectile event test OK\n";
    return 0;
}