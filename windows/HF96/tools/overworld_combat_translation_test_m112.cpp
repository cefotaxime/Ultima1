#include "OriginalData.h"
#include "LocalSimulation.h"
#include "OverworldSimulation.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M112 overworld combat translation FAIL: "<<s<<"\n";
    std::exit(1);
}
bool containsExact(const std::vector<std::string>& v,const std::string& s){
    for(const auto& x:v) if(x==s) return true;
    return false;
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
    p.agility=100;  // player hit check always succeeds
    p.stamina=0;
    p.readyArmour=0;
    p.readyWeapon=15; // Blaster
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

    // Nonlethal player hit: exact translated structure.
    {
        auto p=basePlayer(1000);
        sim.seed(1);
        const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
        if(out.size()!=1) fail("nonlethal hit line count");
        if(out[0].find("네스호 괴수에게 적중! ")!=0 &&
           out[0].find("네스호 괴수에게 적중! ")!=0)
            fail("player hit wording");
        if(out[0].find("의 피해량!")==std::string::npos)
            fail("player damage suffix");
        if(containsPart(out,"Hit the ") || containsPart(out," damage"))
            fail("old player hit wording remains");
    }

    // Lethal hit: exact kill wording and localized copper/silver reward units.
    {
        bool foundBoth=false;
        for(unsigned seed=1;seed<=1024 && !foundBoth;++seed){
            auto p=basePlayer(1);
            p.coin=0;
            sim.seed(seed);
            const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
            if(!containsPart(out,"네스호 괴수를 처치했습니다!"))
                fail("kill wording");
            if(containsPart(out," copper!") || containsPart(out," silver!"))
                fail("old coin reward wording remains");
            if(containsPart(out,"동화 ") && containsPart(out,"은화 ")){
                bool combined=false;
                for(const auto& line:out){
                    if(line.find("동화 ")!=std::string::npos &&
                       line.find("은화 ")!=std::string::npos)
                        combined=true;
                    if(line.find("동화 ")!=std::string::npos &&
                       line.find("냥!")==std::string::npos)
                        fail("copper 냥 suffix");
                    if(line.find("은화 ")!=std::string::npos &&
                       line.find("냥!")==std::string::npos)
                        fail("silver 냥 suffix");
                }
                if(!combined) fail("copper/silver not combined on one logical row");
                if(out.size()!=2) fail("M119 lethal result must use hit row + kill/loot row");
                foundBoth=true;
            }
        }
        if(!foundBoth) fail("could not exercise both copper and silver rewards");
    }

    // Monster successful hit: original two fragments become one localized row.
    {
        bool foundHit=false;
        for(unsigned seed=1;seed<=512 && !foundHit;++seed){
            auto p=basePlayer(100);
            p.readyWeapon=0;
            p.agility=0;
            p.strength=0;
            p.hits=1000;
            p.stamina=0;
            sim.seed(seed);
            const auto out=world.updateAfterCommand(p);
            for(const auto& line:out){
                if(line.find("의 공격! 적중! ")!=std::string::npos ||
                   line.find("의 공격! 적중! ")!=std::string::npos){
                    if(line.find("의 피해량!")==std::string::npos)
                        fail("monster damage suffix");
                    if(line.find('@')!=std::string::npos ||
                       line.find('[')!=std::string::npos ||
                       line.find(']')!=std::string::npos)
                        fail("particle template leaked");
                    foundHit=true;
                }
            }
            if(foundHit){
                if(containsExact(out,"Ness creature attacks!"))
                    fail("separate English attacks row remains on hit");
                if(containsPart(out,"Hit! "))
                    fail("separate English Hit row remains on hit");
            }
        }
        if(!foundHit) fail("could not exercise monster successful-hit path");
    }

    std::cout<<"M112/M114 overworld combat/reward translation test OK\n";
    return 0;
}