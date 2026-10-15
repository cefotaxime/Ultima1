#include "CommandSystem.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M102 Steal/Ready FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    OriginalData original;
    LocalSimulation sim(original);

    // Local Steal on a non-storeroom tile must now return one exact command-line
    // string so main can retain the blue > on that same row.
    LocalState local;
    local.active=true;
    local.inTown=false;
    local.tilesInitialized=true;
    local.tiles.fill(1);
    local.x=0;
    local.y=0;

    PlayerSave p;
    auto steal=sim.steal(local,p);
    if(steal.size()!=1 ||
       steal[0]!="훔치기 - 여기는 훔칠 것이 없습니다!")
        fail("local nothing-here Steal result mismatch");

    // Ready category: any non-W/A/S choice ends with the user-approved "안 한다".
    CommandSystem cmd;
    std::vector<std::string> out;
    if(cmd.beginReady(p,out)) fail("Ready begin completed unexpectedly");
    out.clear();
    if(!cmd.handleKey('X',local,p,sim,out))
        fail("Ready invalid choice did not complete");
    if(out.size()!=1 || out[0]!="안 한다")
        fail("Ready invalid choice did not emit 안 한다");
    if(cmd.active())
        fail("Ready remained active after 안 한다");

    std::cout<<"M102 Steal prompt/nothing-here + Ready none test OK\n";
    return 0;
}