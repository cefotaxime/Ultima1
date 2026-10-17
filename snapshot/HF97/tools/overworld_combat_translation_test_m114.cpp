#include "OriginalData.h"
#include "LocalSimulation.h"
#include "OverworldSimulation.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M114 overworld combat translation FAIL: "<<s<<"\n";
    std::exit(1);
}
bool containsPart(const std::vector<std::string>& v,const std::string& part){
    for(const auto& x:v) if(x.find(part)!=std::string::npos) return true;
    return false;
}
PlayerSave basePlayer(std::uint16_t targetHp){
    PlayerSave p;
    p.x=10; p.y=10;
    p.hits=1000;
    p.strength=25;
    p.agility=100;
    p.stamina=0;
    p.readyArmour=0;
    p.readyWeapon=15;
    p.worldObjects[1].w[0]=19; // Ness creature
    p.worldObjects[1].w[2]=11;
    p.worldObjects[1].w[3]=10;
    p.worldObjects[1].w[4]=targetHp;
    return p;
}
}

int main(int argc,char** argv){
    if(argc!=2) fail("usage: test GAMEDATA");

    OriginalData original;
    std::string error;
    if(!original.loadGameplayOracle(argv[1],error))
        fail("could not load original gameplay data: "+error);

    LocalSimulation sim(original);
    OverworldSimulation world(original,sim);

    // Revised player-hit wording.
    {
        auto p=basePlayer(1000);
        sim.seed(1);
        const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
        if(out.size()!=1) fail("player hit line count");
        if(out[0].find("네스호 괴수에게 적중! ")!=0)
            fail("short player hit wording");
        if(out[0].find("의 피해량!")==std::string::npos)
            fail("player hit damage suffix");
        if(containsPart(out,"공격이 적중했습니다"))
            fail("old long player-hit wording remains");
    }

    // Exact "Missed the [target]!" translation path.
    {
        bool foundMiss=false;
        for(unsigned seed=1;seed<=1024 && !foundMiss;++seed){
            auto p=basePlayer(1000);
            p.agility=0; // allow the hit-roll failure branch
            sim.seed(seed);
            const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
            if(out.size()==1 && out[0].find("빗맞힘!")==out[0].size()-std::string("빗맞힘!").size()){
                if(out[0].find("Missed the ")!=std::string::npos)
                    fail("English missed-target wording remains");
                if(out[0].find('@')!=std::string::npos ||
                   out[0].find('[')!=std::string::npos ||
                   out[0].find(']')!=std::string::npos)
                    fail("particle template leaked on miss");
                foundMiss=true;
            }
        }
        if(!foundMiss) fail("could not exercise missed-target path");
    }

    // Kill reward denominations must share one logical message row.
    {
        bool foundBoth=false;
        for(unsigned seed=1;seed<=2048 && !foundBoth;++seed){
            auto p=basePlayer(1);
            p.coin=0;
            sim.seed(seed);
            const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
            if(out.size()!=2 ||
               out[1].find("네스호 괴수를 처치했습니다!")==std::string::npos)
                fail("kill wording / M119 row count");

            if(out[1].find("동화 ")!=std::string::npos &&
               out[1].find("은화 ")!=std::string::npos){
                const auto c=out[1].find("동화 ");
                const auto si=out[1].find("은화 ");
                if(!(c<si && out[1].find("냥! ",c)!=std::string::npos))
                    fail("reward denominations are not adjacent on kill/loot row");
                foundBoth=true;
            }
        }
        if(!foundBoth) fail("could not exercise combined copper/silver reward");
    }

    // Revised monster successful-hit wording.
    {
        bool foundHit=false;
        for(unsigned seed=1;seed<=1024 && !foundHit;++seed){
            auto p=basePlayer(100);
            p.readyWeapon=0;
            p.agility=0;
            p.strength=0;
            p.hits=1000;
            p.stamina=0;
            sim.seed(seed);
            const auto out=world.updateAfterCommand(p);
            for(const auto& line:out){
                if(line.find("의 공격! 적중! ")!=std::string::npos){
                    if(line.find("의 피해량!")==std::string::npos)
                        fail("monster-hit damage suffix");
                    if(line.find("공격이 적중했습니다")!=std::string::npos)
                        fail("old long monster-hit wording remains");
                    foundHit=true;
                }
            }
        }
        if(!foundHit) fail("could not exercise monster successful-hit path");
    }

    std::cout<<"M114 shortened combat text + inline reward test OK\n";
    return 0;
}
