#include "KoreanText.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool contains(const std::vector<std::string>& v,const std::string& s){
    return std::find(v.begin(),v.end(),s)!=v.end();
}
bool startsWith(const std::string& s,const std::string& p){
    return s.size()>=p.size() && s.compare(0,p.size(),p)==0;
}
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M95 castle combat FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    // Attack prompt grammar must use the M94 particle processor, including ㄹ exception.
    const std::vector<std::pair<int,std::string>> weaponPhrases={
        {0,"맨손으로 공격하기"},
        {1,"단검으로 공격하기"},
        {2,"철퇴로 공격하기"},
        {3,"도끼로 공격하기"},
        {4,"밧줄과 못으로 공격하기"},
        {5,"검으로 공격하기"},
        {6,"대검으로 공격하기"},
        {7,"활과 화살로 공격하기"},
        {8,"수호부로 공격하기"},
        {9,"지팡이로 공격하기"},
        {10,"마법봉으로 공격하기"},
        {11,"삼각검으로 공격하기"},
        {12,"권총으로 공격하기"},
        {13,"광선검으로 공격하기"},
        {14,"페이저로 공격하기"},
        {15,"블라스터로 공격하기"},
    };
    for(const auto& [idx,expected]:weaponPhrases){
        const std::string input="["+std::string(무기_이름_한글(idx))+"]@으로 공격하기";
        const std::string got=조사_처리기(input);
        if(got!=expected) fail("weapon particle phrase mismatch: "+got);
    }

    OriginalData original;
    LocalSimulation sim(original);

    // Castle player attack: guaranteed nonlethal hit against adjacent Guard.
    LocalState castle;
    castle.active=true;
    castle.inTown=false;
    castle.tilesInitialized=true;
    castle.tiles.fill(1);
    castle.x=10; castle.y=10;
    castle.actors[0].tile=0x11; // Guard
    castle.actors[0].x=11;
    castle.actors[0].y=10;
    castle.actors[0].value=9999;

    PlayerSave p;
    p.readyWeapon=1; // Dagger
    p.agility=99;    // Agility+50 > 100, therefore guaranteed hit.
    p.strength=16;
    sim.seed(0x1234);

    auto hit=sim.attack(castle,p,1,0);
    if(hit.size()!=1 || !startsWith(hit[0],"경비병에게 적중! ") ||
       hit[0].find("의 피해량!")==std::string::npos)
        fail("castle hit wording");

    // Castle miss wording.
    castle.actors[0].tile=0xFFFF;
    castle.alert=false;
    sim.seed(0x1234);
    auto miss=sim.attack(castle,p,1,0);
    if(miss.size()!=1 || miss[0]!="공격이 빗나갑니다!")
        fail("castle miss wording");

    // Scope guard: town combat remains on the original English path for now.
    LocalState town=castle;
    town.inTown=true;
    sim.seed(0x1234);
    auto townMiss=sim.attack(town,p,1,0);
    if(townMiss.size()!=1 || townMiss[0]!="공격이 빗나갑니다!")
        fail("town wording changed outside requested castle scope");

    // Castle Guard retaliation: huge Stamina guarantees the guard misses.
    LocalState retaliation;
    retaliation.active=true;
    retaliation.inTown=false;
    retaliation.alert=true;
    retaliation.tilesInitialized=true;
    retaliation.tiles.fill(1);
    retaliation.x=10; retaliation.y=10;
    retaliation.actors[0].tile=0x11;
    retaliation.actors[0].x=11;
    retaliation.actors[0].y=10;
    retaliation.actors[0].value=100;

    PlayerSave defender;
    defender.hits=9999;
    defender.stamina=9999;
    defender.readyArmour=0;
    sim.seed(0x4567);
    auto guard=sim.updateAfterCommand(retaliation,defender);
    if(guard.size()!=1 ||
       guard[0]!="경비병의 공격! 공격이 빗맞습니다!")
        fail("castle guard retaliation wording");

    std::cout<<"M95 castle combat localization test OK\n";
    return 0;
}