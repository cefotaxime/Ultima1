#include "OriginalData.h"
#include "LocalSimulation.h"
#include "OverworldSimulation.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M119 combat message layout FAIL: "<<s<<"\n";
    std::exit(1);
}
bool has(const std::string& s,const std::string& q){
    return s.find(q)!=std::string::npos;
}
PlayerSave fieldPlayer(std::uint16_t hp){
    PlayerSave p;
    p.x=10; p.y=10;
    p.hits=1000;
    p.strength=25;
    p.agility=100;
    p.readyWeapon=15;
    p.worldObjects[1].w[0]=19; // 네스호 괴수
    p.worldObjects[1].w[2]=11;
    p.worldObjects[1].w[3]=10;
    p.worldObjects[1].w[4]=hp;
    return p;
}
LocalState castleWithGuard(std::uint16_t hp){
    LocalState c;
    c.active=true;
    c.inTown=false;
    c.tilesInitialized=true;
    c.tiles.fill(1);
    c.x=10; c.y=10;
    c.actors[0].tile=0x11;
    c.actors[0].x=11;
    c.actors[0].y=10;
    c.actors[0].value=hp;
    return c;
}
}

int main(int argc,char** argv){
    if(argc!=2) fail("usage: test GAMEDATA");

    OriginalData original;
    std::string error;
    if(!original.loadGameplayOracle(argv[1],error))
        fail("could not load original gameplay oracle: "+error);

    LocalSimulation rng(original);
    OverworldSimulation world(original,rng);

    // Field lethal attack: hit/damage row first; kill+loot starts next row.
    {
        bool foundCopperSilver=false;
        for(unsigned seed=1;seed<=4096 && !foundCopperSilver;++seed){
            auto p=fieldPlayer(1);
            p.coin=0;
            rng.seed(static_cast<std::uint16_t>(seed));
            const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
            if(out.size()!=2) fail("field lethal result is not exactly two logical rows");
            if(out[0].find("네스호 괴수에게 적중! ")!=0 ||
               !has(out[0],"의 피해량!"))
                fail("field lethal hit row");
            if(out[1].find("네스호 괴수를 처치했습니다! ")!=0 &&
               out[1]!="네스호 괴수를 처치했습니다!")
                fail("field kill row prefix");
            if(has(out[0],"처치했습니다!") ||
               has(out[0],"동화 ") || has(out[0],"은화 "))
                fail("kill/loot leaked onto field hit row");

            const auto copper=out[1].find("동화 ");
            const auto silver=out[1].find("은화 ");
            if(copper!=std::string::npos && silver!=std::string::npos){
                if(!(copper<silver)) fail("field loot order");
                if(out[1].find("냥! ",copper)==std::string::npos)
                    fail("field loot not continuous on kill row");
                foundCopperSilver=true;
            }
        }
        if(!foundCopperSilver) fail("could not exercise field copper+silver reward");
    }

    // Castle nonlethal hit: compact one-row wording.
    {
        auto c=castleWithGuard(9999);
        PlayerSave p;
        p.readyWeapon=1;
        p.agility=99;
        p.strength=16;
        rng.seed(0x1234);
        const auto out=rng.attack(c,p,1,0);
        if(out.size()!=1 ||
           out[0].find("경비병에게 적중! ")!=0 ||
           !has(out[0],"의 피해량!"))
            fail("castle nonlethal hit wording");
        if(has(out[0],"피해를 입힙니다"))
            fail("old castle hit wording remains");
    }

    // Castle lethal hit: kill acknowledgement is explicitly the next row.
    {
        auto c=castleWithGuard(1);
        PlayerSave p;
        p.readyWeapon=1;
        p.agility=99;
        p.strength=99;
        rng.seed(0x2222);
        const auto out=rng.attack(c,p,1,0);
        if(out.size()!=2) fail("castle lethal result must use two logical rows");
        if(out[0].find("경비병에게 적중! ")!=0 ||
           !has(out[0],"의 피해량!"))
            fail("castle lethal hit/damage row");
        if(out[1]!="처치했습니다!")
            fail("castle kill acknowledgement row");
    }

    // Castle miss remains the short player-side wording.
    {
        auto c=castleWithGuard(9999);
        c.actors[0].tile=0xFFFF;
        PlayerSave p;
        p.readyWeapon=1;
        p.agility=99;
        p.strength=16;
        rng.seed(0x3333);
        const auto out=rng.attack(c,p,1,0);
        if(out.size()!=1 || out[0]!="공격이 빗나갑니다!")
            fail("castle player miss wording");
    }

    // Guard miss: one compact enemy row, distinct from player row.
    {
        auto c=castleWithGuard(100);
        c.alert=true;
        PlayerSave p;
        p.hits=9999;
        p.stamina=9999;
        p.readyArmour=0;
        rng.seed(0x4567);
        const auto out=rng.updateAfterCommand(c,p);
        if(out.size()!=1 ||
           out[0]!="경비병의 공격! 공격이 빗맞습니다!")
            fail("castle guard miss stream");
    }

    // Guard hit: attack + hit + damage remain one logical row.
    {
        bool found=false;
        for(unsigned seed=1;seed<=1024 && !found;++seed){
            auto c=castleWithGuard(100);
            c.alert=true;
            PlayerSave p;
            p.hits=1000;
            p.stamina=0;
            p.readyArmour=0;
            rng.seed(static_cast<std::uint16_t>(seed));
            const auto out=rng.updateAfterCommand(c,p);
            if(out.size()!=1) continue;
            if(out[0].find("경비병의 공격! 공격 적중!...")==0 &&
               has(out[0],"의 피해량!")){
                if(has(out[0],"피해를 입습니다"))
                    fail("old guard damage wording remains");
                found=true;
            }
        }
        if(!found) fail("could not exercise castle guard hit stream");
    }

    std::cout<<"M119 field/castle combat message layout test OK\n";
    return 0;
}