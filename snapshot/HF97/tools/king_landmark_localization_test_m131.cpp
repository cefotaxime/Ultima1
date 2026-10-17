#include "KingSystem.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M131 king/landmark localization FAIL: "<<s<<"\n";
    std::exit(1);
}
bool has(const std::vector<std::string>& v,const std::string& q){
    return std::find(v.begin(),v.end(),q)!=v.end();
}
bool starts(const std::vector<std::string>& v,const std::string& q){
    for(const auto& s:v) if(s.rfind(q,0)==0) return true;
    return false;
}
bool adjacent(const std::vector<std::string>& v,const std::string& a,const std::string& b){
    for(std::size_t i=0;i+1<v.size();++i) if(v[i]==a && v[i+1]==b) return true;
    return false;
}
}

int main(){
    PlayerSave p;
    p.agility=50;
    p.stamina=50;
    p.charisma=50;
    p.wisdom=50;
    p.intelligence=50;
    p.weapons.fill(1);
    p.weapons[5]=0; // first missing weapon = Sword -> 검
    int last=-1;
    std::vector<std::string> m;

    // 41: Pillars of Protection -> Agility +5.
    KingSystem::visitLandmark(41,p,last,m);
    if(!has(m,"강력한 마법의 기운이 당신을 에워쌉니다!")) fail("Protection text");
    if(!has(m,"당신의 민첩이 5만큼 증가합니다.")) fail("Agility reward");
    if(p.agility!=55) fail("Agility mutation");

    // 42: Tower of Knowledge -> Intelligence +5.
    m.clear();
    KingSystem::visitLandmark(42,p,last,m);
    if(!adjacent(m,"누군가가 \"Turis-scientia-magnopere\"라고 말합니다.","(역자주: 지혜의 탑의 위업)")) fail("Tower translator-note line break");
    if(!has(m,"당신의 지능이 5만큼 증가합니다.")) fail("Intelligence reward");

    // 43: Pillars of Argonauts -> first missing weapon, with Korean particle.
    m.clear();
    KingSystem::visitLandmark(43,p,last,m);
    if(!adjacent(m,"표지판에는 \"Fortes fortuna adiuvat!\"라고 써 있습니다.","(역자주: 행운은 용자를 돕는다)")) fail("Argonaut translator-note line break");
    if(!has(m,"당신은 검을 발견합니다.")) fail("weapon reward");
    if(p.weapons[5]!=1) fail("weapon mutation");

    // 44: Ozymandias -> Wisdom +5.
    m.clear();
    KingSystem::visitLandmark(44,p,last,m);
    if(!adjacent(m,"대좌 위에 글자가 나타납니다.","\"짐은 왕 중의 왕 오지만디아스다. 강한 이여, 짐의 위업을 보고 절망하라!\"")) fail("Ozymandias quote line break");
    if(!has(m,"그 외에는 아무것도 없습니다. 당신은 기묘한 기운을 느낍니다!")) fail("Ozymandias second text");
    if(!has(m,"당신의 지혜가 5만큼 증가합니다.")) fail("Wisdom reward");

    // 45: Sign Post -> Stamina +5.
    m.clear();
    KingSystem::visitLandmark(45,p,last,m);
    if(!adjacent(m,"표지판에는 \"Ultima thule!\"라고 써 있습니다.","(역자주: 미지의 땅)")) fail("Ultima thule translator-note line break");
    if(!has(m,"하늘이 어두워지고, 강력한 마법이 당신을 집어삼킵니다!")) fail("dark sky");
    if(!has(m,"당신의 지구력이 5만큼 증가합니다.")) fail("Stamina reward");

    // 46: Southern Sign Post -> Charisma +5.
    m.clear();
    KingSystem::visitLandmark(46,p,last,m);
    if(!adjacent(m,"표지판에는 \"Omnia mutantur!\"라고 써 있습니다.","(역자주: 만물은 변화한다)")) fail("Omnia translator-note line break");
    if(!has(m,"당신의 매력이 5만큼 증가합니다.")) fail("Charisma reward");

    // 47: Eastern Sign Post -> text only.
    m.clear();
    KingSystem::visitLandmark(47,p,last,m);
    if(!has(m,"표지판에는 \"동쪽으로 가려면 동쪽으로 가라!\"라고 써 있습니다.")) fail("east sign");

    // 48: Grave -> Stamina +5 again (55 -> 6 by original formula (55+8)/10).
    m.clear();
    KingSystem::visitLandmark(48,p,last,m);
    if(!adjacent(m,"묘비에 \"Vae Victis\"라고 새겨져 있습니다.","(역자주: 패배한 자에게 재앙 있으리라)")) fail("grave translator-note line break");
    if(!has(m,"당신의 지구력이 6만큼 증가합니다.")) fail("second Stamina reward");
    if(p.stamina!=61) fail("second Stamina mutation");

    // Same landmark twice: flavor text still appears, reward is suppressed.
    m.clear();
    const auto before=p.stamina;
    KingSystem::visitLandmark(48,p,last,m);
    if(p.stamina!=before) fail("repeat landmark rewarded again");
    if(!has(m,"흐음...아무런 효과도 없습니다!")) fail("repeat landmark no-effect");

    // Odd landmark quest completion happens before reward suppression.
    p.kingQuest[7]=0xFFFFu; // index 1 -> castleIndex 7
    last=-1;
    m.clear();
    KingSystem::visitLandmark(42,p,last,m);
    if(p.kingQuest[7]!=1) fail("landmark quest completion state");
    if(!has(m,"임무를 완료하였습니다!")) fail("quest completion notice");

    std::cout<<"M131 landmark localization/reward regression OK\n";
    return 0;
}
