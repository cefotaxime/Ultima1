#include "KoreanText.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M96 actor translation FAIL: "<<s<<"\n";
    std::exit(1);
}
bool contains(const std::vector<std::string>& v,const std::string& s){
    return std::find(v.begin(),v.end(),s)!=v.end();
}
bool startsWithAny(const std::vector<std::string>& v,const std::string& prefix){
    for(const auto& line:v)
        if(line.size()>=prefix.size() && line.compare(0,prefix.size(),prefix)==0)
            return true;
    return false;
}
}

int main(){
    const std::vector<std::pair<std::string,std::string>> mappings={
        {인물_이름_한글(0x14,false),"국왕"},
        {인물_이름_한글(0x11,false),"경비병"},
        {인물_이름_한글(0x15,false),"상인"},
        {인물_이름_한글(0x16,false),"공주"},
        {인물_이름_한글(0x13,true),"음유시인"},
        {인물_이름_한글(0x13,false),"광대"},
        {인물_이름_한글(0x32,true),"꽃뱀"},
        {인물_이름_한글(0x32,false),"제비"},
    };
    for(const auto& [got,expected]:mappings){
        if(got!=expected) fail("mapping mismatch: expected "+expected+", got "+got);
    }

    // Direct regression for the M95 mistake: castle King must display 국왕, never 왕.
    OriginalData original;
    LocalSimulation sim(original);

    LocalState castle;
    castle.active=true;
    castle.inTown=false;
    castle.locationId=33; // Lord British; ordinary damage is allowed here too.
    castle.tilesInitialized=true;
    castle.tiles.fill(1);
    castle.x=10;
    castle.y=10;
    castle.actors[0].tile=0x14; // King
    castle.actors[0].x=11;
    castle.actors[0].y=10;
    castle.actors[0].value=1;   // force kill branch

    PlayerSave p;
    p.readyWeapon=1;
    p.agility=99;   // guaranteed hit because hit chance >100
    p.strength=99;
    sim.seed(0x2222);

    const auto out=sim.attack(castle,p,1,0);
    // M119 appends the damage to the same hit row:
    //   "국왕에게 적중! N의 피해량!"
    // Therefore this actor-name regression must check the prefix rather than
    // require the old pre-M119 exact string "국왕에게 적중!".
    if(!startsWithAny(out,"국왕에게 적중!")) fail("King hit did not use 국왕");
    if(!contains(out,"처치했습니다!")) fail("Killed! did not translate to 처치했습니다!");
    for(const auto& line:out){
        if(line.find("왕에게 적중!")!=std::string::npos &&
           line.find("국왕에게 적중!")==std::string::npos)
            fail("unauthorized 왕 translation still visible");
        if(line=="Killed!") fail("English Killed! still visible");
    }

    std::cout<<"M109 actor-name/Killed localization test OK\n";
    return 0;
}
