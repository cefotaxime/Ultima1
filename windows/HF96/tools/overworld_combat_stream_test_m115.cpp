#include "OriginalData.h"
#include "LocalSimulation.h"
#include "OverworldSimulation.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M115 overworld combat stream FAIL: "<<s<<"\n";
    std::exit(1);
}

PlayerSave playerWithMonster(std::uint16_t hp,int tile=19){
    PlayerSave p;
    p.x=10; p.y=10;
    p.hits=1000;
    p.strength=25;
    p.agility=100;
    p.stamina=0;
    p.readyArmour=0;
    p.readyWeapon=15; // Blaster; range > 0
    p.worldObjects[1].w[0]=static_cast<std::uint16_t>(tile);
    p.worldObjects[1].w[2]=11;
    p.worldObjects[1].w[3]=10;
    p.worldObjects[1].w[4]=hp;
    return p;
}

bool has(const std::string& s,const std::string& q){
    return s.find(q)!=std::string::npos;
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

    // User-supplied field monster translations, including M116 Thief=도적.
    const std::array<const char*,15> expectedNames={
        "네스호 괴수","거대 오징어","거북용","해적선","강도",
        "곰","은신 궁수","암흑 기사","사악한 나무 거인","도적","오크",
        "기사","강령술사","타락한 레인저","떠돌이 흑마술사"
    };
    for(int i=0;i<15;++i){
        const int tile=19+i*2;
        if(std::string(OverworldSimulation::monsterName(tile))!=expectedNames[static_cast<std::size_t>(i)])
            fail("monster name table mismatch at index "+std::to_string(i));
    }

    // 1) Empty-space attack: distinct Missed! translation.
    {
        PlayerSave p;
        p.x=10; p.y=10;
        p.strength=25; p.agility=100; p.readyWeapon=15;
        sim.seed(1);
        const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
        if(out.size()!=1 || out[0]!="공격이 빗나갑니다!")
            fail("empty-space Missed! wording");
    }

    // 2a) Targeted miss: one logical output stream.
    {
        bool found=false;
        for(unsigned seed=1;seed<=2048 && !found;++seed){
            auto p=playerWithMonster(1000);
            p.agility=0;
            sim.seed(seed);
            const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
            if(out.size()==1 && out[0]=="네스호 괴수를 빗맞힘!")
                found=true;
        }
        if(!found) fail("could not exercise targeted miss");
    }

    // 2b) Nonlethal hit: one logical output stream, no legacy wording.
    {
        auto p=playerWithMonster(1000);
        sim.seed(1);
        const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
        if(out.size()!=1) fail("nonlethal player hit split into multiple logical rows");
        if(out[0].find("네스호 괴수에게 적중! ")!=0 ||
           !has(out[0],"의 피해량!"))
            fail("nonlethal hit wording");
        if(has(out[0],"공격이 적중") || has(out[0],"피해를 입힙니다"))
            fail("legacy long player-hit wording remains");
    }

    // 2c) M119 lethal hit: hit/damage ends the first logical row.
    // Kill + all loot begin on the next logical row and stay continuous there.
    {
        bool foundCopperSilver=false;
        for(unsigned seed=1;seed<=4096 && !foundCopperSilver;++seed){
            auto p=playerWithMonster(1);
            p.coin=0;
            sim.seed(seed);
            const auto out=world.attack(p,1,0,WorldAttackMode::Normal);
            if(out.size()!=2) fail("M119 lethal result must use exactly two logical rows");

            const auto& hitLine=out[0];
            const auto& killLine=out[1];
            if(hitLine.find("네스호 괴수에게 적중! ")!=0 ||
               hitLine.find("의 피해량!")==std::string::npos)
                fail("lethal hit/damage row");
            if(killLine.find("네스호 괴수를 처치했습니다!")==0){
                const auto copper=killLine.find("동화 ");
                const auto silver=killLine.find("은화 ");
                if(copper!=std::string::npos && silver!=std::string::npos){
                    if(!(copper<silver))
                        fail("loot ordering");
                    if(killLine.find("냥! ",copper)==std::string::npos)
                        fail("loot fragments not adjacent on kill row");
                    foundCopperSilver=true;
                }
            }else{
                fail("kill/loot row does not start with kill message");
            }
        }
        if(!foundCopperSilver) fail("could not exercise copper+silver lethal reward");
    }

    // 3) One enemy attack is one logical stream whether hit or miss.
    {
        bool foundHit=false,foundMiss=false;
        for(unsigned seed=1;seed<=4096 && !(foundHit&&foundMiss);++seed){
            auto p=playerWithMonster(100);
            p.readyWeapon=0;
            p.hits=1000;
            p.stamina=0;
            p.readyArmour=0;
            sim.seed(seed);
            const auto out=world.updateAfterCommand(p);
            if(out.size()!=1) fail("one monster attack split into multiple logical rows");
            const auto& line=out[0];
            if(line.find("네스호 괴수의 공격! ")!=0)
                fail("enemy attack prefix");
            if(has(line,"적중! ")){
                if(!has(line,"의 피해량!")) fail("enemy hit damage wording");
                foundHit=true;
            }else if(line=="네스호 괴수의 공격! 빗맞음!"){
                foundMiss=true;
            }else{
                fail("unexpected enemy attack result: "+line);
            }
            if(has(line," attacks!") || has(line,"Missed!") || has(line,"Hit!"))
                fail("legacy English enemy fragments remain");
        }
        if(!foundHit || !foundMiss) fail("did not exercise both enemy hit and miss");
    }

    std::cout<<"M115 four-line-conscious overworld combat stream test OK\n";
    return 0;
}