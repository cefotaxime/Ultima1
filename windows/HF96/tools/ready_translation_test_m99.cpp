#include "CommandSystem.h"
#include "KoreanText.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M99 Ready translation FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    OriginalData original;
    LocalSimulation sim(original);
    LocalState local;
    PlayerSave p;
    CommandSystem cmd;
    std::vector<std::string> out;

    // Weapon popup prompt + corrected Bow & Arrows translation.
    p.weapons.fill(0);
    p.weapons[7]=1;
    cmd.beginReady(p,out);
    out.clear();
    if(cmd.handleKey('W',local,p,sim,out)) fail("W did not open popup");
    if(cmd.inputLine()!="무기 장비하기:  ") fail("weapon prompt mismatch");
    auto weapons=cmd.readyMenuEntries(p);
    if(weapons.size()!=1 || weapons[0]!=std::make_pair('h',std::string("활과 화살")))
        fail("Bow & Arrows translation mismatch");
    if(std::string(무기_이름_한글(7))!="활과 화살")
        fail("central weapon translation mismatch");
    cmd.cancel();

    // Armour popup prompt + exact user-provided armour translations.
    p.armour.fill(1);
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    if(cmd.handleKey('A',local,p,sim,out)) fail("A did not open popup");
    if(cmd.inputLine()!="방어구 장비하기:  ") fail("armour prompt mismatch");
    const std::vector<std::pair<char,std::string>> expectedArmour={
        {'a',"맨몸"},
        {'b',"가죽 갑옷"},
        {'c',"사슬 갑옷"},
        {'d',"판금 갑옷"},
        {'e',"우주복"},
        {'f',"반사복"},
    };
    if(cmd.readyMenuEntries(p)!=expectedArmour) fail("armour names mismatch");
    cmd.cancel();

    // Spell popup: M100 applies user-approved translations, while Steal
    // deliberately remains untranslated.
    p.spells.fill(1);
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    if(cmd.handleKey('S',local,p,sim,out)) fail("S did not open popup");
    if(cmd.inputLine()!="주문 장비하기:  ") fail("spell prompt mismatch");
    const std::vector<std::pair<char,std::string>> expectedSpells={
        {'a',"기도"},
        {'b',"관 열기"},
        {'c',"상자 열기"},
        {'d',"마탄"},
        {'e',"훔치기"},
        {'f',"상승 사다리"},
        {'g',"하강 사다리"},
        {'h',"순간이동"},
        {'i',"마력장 생성"},
        {'j',"마력장 파괴"},
        {'k',"죽음"},
    };
    if(cmd.readyMenuEntries(p)!=expectedSpells)
        fail("spell names mismatch");

    std::cout<<"M100 Ready/armour/Bow/spell translation test OK\n";
    return 0;
}