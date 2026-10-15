#include "CommandSystem.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M97 Ready popup FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    CommandSystem cmd;
    LocalState local;
    PlayerSave p;
    OriginalData original;
    LocalSimulation sim(original); // Ready branches do not consult loaded graphics/data.

    // Owned inventory chosen to prove original index letters are preserved.
    p.weapons.fill(0);
    p.weapons[0]=1; // a) Hands / 맨손
    p.weapons[1]=1; // b) Dagger / 단검
    p.weapons[7]=1; // h) Bow / 활
    p.readyWeapon=1;

    std::vector<std::string> out;
    if(cmd.beginReady(p,out)) fail("beginReady completed immediately");
    if(!out.empty()) fail("Ready category was incorrectly committed to log");
    if(cmd.state()!=CommandSystem::State::ReadyCategory) fail("not in ReadyCategory");
    if(cmd.inputLine()!="무기(W), 방어구(A), 주문(S) 장비하기:  ")
        fail("Korean Ready prompt mismatch");
    if(cmd.inputLine().size()<2 ||
       cmd.inputLine().substr(cmd.inputLine().size()-2)!="  ")
        fail("Ready prompt cursor gap is not two spaces");
    if(cmd.readyPopupActive()) fail("popup opened before W/A/S");

    out.clear();
    if(cmd.handleKey('W',local,p,sim,out)) fail("W selection completed instead of opening popup");
    if(cmd.state()!=CommandSystem::State::ReadyWeapon) fail("W did not enter ReadyWeapon");
    if(!cmd.readyPopupActive()) fail("weapon popup not active");
    if(cmd.inputLine()!="무기 장비하기:  ") fail("weapon live line mismatch");
    if(cmd.readyCursorColumn()!=-1) fail("M121 localized Ready cursor must follow measured prompt endpoint");
    if(!out.empty()) fail("W selection emitted a separate log row");

    const auto entries=cmd.readyMenuEntries(p);
    if(entries.size()!=3) fail("owned weapon entry count mismatch");
    if(entries[0]!=std::make_pair('a',std::string("맨손"))) fail("a) 맨손 missing");
    if(entries[1]!=std::make_pair('b',std::string("단검"))) fail("b) 단검 missing");
    if(entries[2]!=std::make_pair('h',std::string("활과 화살"))) fail("h) 활 missing");

    out.clear();
    if(!cmd.handleKey('h',local,p,sim,out)) fail("weapon selection did not finish");
    if(cmd.active()) fail("Ready remained active after item selection");
    if(p.readyWeapon!=7) fail("readyWeapon was not updated to Bow index 7");
    if(out.size()!=1 || out[0]!="활과 화살") fail("selected visible weapon name mismatch");

    // Armour popup.
    p.armour.fill(0);
    p.armour[0]=1;
    p.armour[2]=1;
    p.readyArmour=0;
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    if(cmd.handleKey('A',local,p,sim,out)) fail("A selection completed instead of opening popup");
    if(cmd.state()!=CommandSystem::State::ReadyArmour || !cmd.readyPopupActive())
        fail("armour popup not active");
    auto armour=cmd.readyMenuEntries(p);
    if(armour.size()!=2 || armour[0].first!='a' || armour[1].first!='c')
        fail("armour original index letters not preserved");
    cmd.cancel();

    // Spell popup.
    p.spells.fill(0);
    p.spells[1]=1;
    p.spells[10]=1;
    p.readySpell=1;
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    if(cmd.handleKey('S',local,p,sim,out)) fail("S selection completed instead of opening popup");
    if(cmd.state()!=CommandSystem::State::ReadySpell || !cmd.readyPopupActive())
        fail("spell popup not active");
    auto spells=cmd.readyMenuEntries(p);
    if(spells.size()!=2 || spells[0].first!='b' || spells[1].first!='k')
        fail("spell original index letters not preserved");

    std::cout<<"M97 Ready popup state test OK\n";
    return 0;
}