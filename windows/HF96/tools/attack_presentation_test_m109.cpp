#include "AttackPresentation.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M109 attack presentation FAIL: "<<s<<"\n";
    std::exit(1);
}
bool contains(const std::vector<std::string>& v,const std::string& s){
    for(const auto& x:v) if(x==s) return true;
    return false;
}
bool startsWithAny(const std::vector<std::string>& v,const std::string& prefix){
    for(const auto& x:v)
        if(x.size()>=prefix.size() && x.compare(0,prefix.size(),prefix)==0) return true;
    return false;
}
}

int main(){
    if(WORLD_PROJECTILE_DOS_TICKS!=2) fail("projectile step is not delay(2)");

    // EF3 path: clear cells run to full range.
    auto clear=buildWorldProjectileVisualPath(1,0,3,[](int,int){return 1;});
    if(clear.size()!=3 || clear[0]!=std::make_pair(1,0) ||
       clear[1]!=std::make_pair(2,0) || clear[2]!=std::make_pair(3,0))
        fail("clear projectile path");

    // Mountain 3 and physical object >=19 stop *after* the effect is drawn there.
    auto mountain=buildWorldProjectileVisualPath(0,-1,3,[](int,int sy){return sy==-2?3:1;});
    if(mountain.size()!=2) fail("mountain stop");
    auto monster=buildWorldProjectileVisualPath(-1,0,3,[](int sx,int){return sx==-2?19:1;});
    if(monster.size()!=2) fail("monster stop");

    // Ordinary physical objects below 19 do not stop OUT.EXE EF3's scan.
    auto vehicle=buildWorldProjectileVisualPath(1,0,3,[](int sx,int){return sx==2?18:1;});
    if(vehicle.size()!=3) fail("ordinary-object pass-through");

    OriginalData original;
    LocalSimulation sim(original);

    // User correction: castle kill message is 처치했습니다!.
    LocalState castle;
    castle.active=true;
    castle.inTown=false;
    castle.tilesInitialized=true;
    castle.tiles.fill(1);
    castle.x=10; castle.y=10;
    castle.actors[0].tile=0x11;
    castle.actors[0].x=11; castle.actors[0].y=10;
    castle.actors[0].value=1;

    PlayerSave attacker;
    attacker.readyWeapon=1;
    attacker.agility=99;
    attacker.strength=99;
    sim.seed(0x2222);
    const auto kill=sim.attack(castle,attacker,1,0);
    if(!contains(kill,"처치했습니다!")) fail("castle kill wording");
    if(contains(kill,"죽었습니다!")) fail("old castle kill wording remains");

    // M119 keeps the Hit!... ellipsis inside one compact castle-guard attack stream.
    bool foundHit=false;
    for(unsigned seed=1;seed<=512 && !foundHit;++seed){
        LocalState retaliation;
        retaliation.active=true;
        retaliation.inTown=false;
        retaliation.alert=true;
        retaliation.tilesInitialized=true;
        retaliation.tiles.fill(1);
        retaliation.x=10; retaliation.y=10;
        retaliation.actors[0].tile=0x11;
        retaliation.actors[0].x=11; retaliation.actors[0].y=10;
        retaliation.actors[0].value=100;

        PlayerSave defender;
        defender.hits=1000;
        defender.stamina=0;
        defender.readyArmour=0;
        sim.seed(seed);
        const auto out=sim.updateAfterCommand(retaliation,defender);
        if(startsWithAny(out,"경비병의 공격! 공격 적중!...")) foundHit=true;
    }
    if(!foundHit) fail("could not exercise guard-hit wording");

    std::cout<<"M109 attack direction/projectile/castle wording test OK\n";
    return 0;
}