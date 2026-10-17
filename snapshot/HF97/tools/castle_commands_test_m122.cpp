#include "CommandSystem.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr << "M122 castle commands FAIL: " << s << "\n";
    std::exit(1);
}
LocalState castleAt(std::uint8_t tile){
    LocalState c;
    c.active=true;
    c.inTown=false;
    c.tilesInitialized=true;
    c.tiles.fill(1);
    c.x=10; c.y=10;
    c.tiles[static_cast<std::size_t>(c.x*OriginalData::LOCAL_H+c.y)]=tile;
    return c;
}
}

int main(){
    OriginalData original;
    LocalSimulation sim(original);
    PlayerSave p;

    // M122 correction: Drop Pence is numeric command-line input only.
    {
        CommandSystem cmd;
        auto c=castleAt(1);
        std::vector<std::string> out;
        if(cmd.beginDrop(c,p,out)) fail("Drop begin completed");
        if(cmd.handleKey('P',c,p,sim,out)) fail("P completed instead of entering amount state");
        if(cmd.state()!=CommandSystem::State::DropPence) fail("P did not enter DropPence");
        if(cmd.dropPopupActive() || cmd.selectorPopupActive())
            fail("Drop Pence opened upper popup");
        if(cmd.inputLine()!="코인 던지기:  ") fail("Drop Pence prompt");
    }

    // W/A still use the upper selector popup.
    {
        CommandSystem cmd;
        auto c=castleAt(1);
        std::vector<std::string> out;
        cmd.beginDrop(c,p,out); out.clear();
        cmd.handleKey('W',c,p,sim,out);
        if(!cmd.dropPopupActive() || cmd.state()!=CommandSystem::State::DropWeapon)
            fail("Drop Weapon popup missing");
        cmd.cancel();
        cmd.beginDrop(c,p,out); out.clear();
        cmd.handleKey('A',c,p,sim,out);
        if(!cmd.dropPopupActive() || cmd.state()!=CommandSystem::State::DropArmour)
            fail("Drop Armour popup missing");
    }

    // Castle Get, empty location.
    {
        auto c=castleAt(1);
        int allowance=0;
        const auto out=sim.get(c,p,allowance);
        if(out.size()!=1 || out[0]!="가지기 - 여기는 가질 것이 없습니다!")
            fail("castle Get nothing-here translation");
    }

    // Castle Get on a storeroom marker without king permission.
    {
        auto c=castleAt(0x39);
        int allowance=0;
        const auto out=sim.get(c,p,allowance);
        if(out.size()!=2 || out[0]!="가지기" || out[1]!="국왕이 윤허하지 않았습니다!")
            fail("castle Get permission translation");
    }

    // Castle Unlock prompt is one Korean command row; no split English "what?".
    {
        auto c=castleAt(1);
        const auto out=sim.unlock(c,p);
        if(out.size()!=1 || out[0]!="무슨 자물쇠를 엽니까?")
            fail("castle Unlock prompt translation");
    }

    std::cout << "M122 castle command translations + Drop Pence no-popup test OK\n";
    return 0;
}
