#include "CommandSystem.h"
#include <algorithm>
#include <cctype>

namespace {
constexpr const char* WEAPONS[16]={
    "맨손","단검","철퇴","도끼","밧줄과 못","검","대검","활과 화살",
    "수호부","지팡이","마법봉","삼각검","권총","광선검","페이저","블라스터"};
constexpr const char* ARMOUR[6]={"맨몸","가죽 갑옷","사슬 갑옷","판금 갑옷","우주복","반사복"};
constexpr const char* SPELLS[11]={"기도","관 열기","상자 열기","마탄","훔치기","상승 사다리",
    "하강 사다리","순간이동","마력장 생성","마력장 파괴","죽음"};
}

int CommandSystem::itemIndex(char ch,int maxIndex){
    const unsigned char u=static_cast<unsigned char>(ch);
    const char c=static_cast<char>(std::tolower(u));
    const int i=c-'a';
    return (i>=0 && i<=maxIndex)?i:-1;
}
const char* CommandSystem::weaponName(int i){ return (i>=0&&i<16)?WEAPONS[i]:WEAPONS[0]; }
const char* CommandSystem::armourName(int i){ return (i>=0&&i<6)?ARMOUR[i]:ARMOUR[0]; }
const char* CommandSystem::spellName(int i){ return (i>=0&&i<11)?SPELLS[i]:SPELLS[0]; }

std::string CommandSystem::inputLine() const{
    switch(state_){
        case State::ReadyCategory:
            return "무기(W), 방어구(A), 주문(S) 장비하기:  ";
        case State::ReadyWeapon:
            return "무기 장비하기:  ";
        case State::ReadyArmour:
            return "방어구 장비하기:  ";
        case State::ReadySpell:
            return "주문 장비하기:  ";
        case State::DropCategory:
            return "코인(P), 무기(W), 방어구(A) 버리기:  ";
        case State::DropPence:
            return "코인 던지기:  "+digits_;
        case State::DropWeapon:
            return "무기 버리기:  ";
        case State::DropArmour:
            return "방어구 버리기:  ";
        default:
            return {};
    }
}

int CommandSystem::readyCursorColumn() const{
    // M121: every Ready/Drop prompt owns exactly two literal ASCII spaces
    // after ':' and the live cursor follows the rendered prompt (and any
    // numeric Pence input).  A fixed BIOS col15 would break that contract for
    // localized prompt widths.
    return -1;
}

std::vector<std::pair<char,std::string>> CommandSystem::readyMenuEntries(const PlayerSave& p) const{
    std::vector<std::pair<char,std::string>> out;
    if(state_==State::ReadyWeapon){
        for(int i=0;i<16;++i)
            if(p.weapons[static_cast<std::size_t>(i)]!=0)
                out.push_back({static_cast<char>('a'+i),weaponName(i)});
    }else if(state_==State::ReadyArmour){
        for(int i=0;i<6;++i)
            if(p.armour[static_cast<std::size_t>(i)]!=0)
                out.push_back({static_cast<char>('a'+i),armourName(i)});
    }else if(state_==State::ReadySpell){
        for(int i=0;i<11;++i)
            if(p.spells[static_cast<std::size_t>(i)]!=0)
                out.push_back({static_cast<char>('a'+i),spellName(i)});
    }
    return out;
}

std::vector<std::pair<char,std::string>> CommandSystem::dropMenuEntries(const PlayerSave& p) const{
    std::vector<std::pair<char,std::string>> out;
    if(state_==State::DropWeapon){
        // Index 0 is Hands and cannot be dropped.
        for(int i=1;i<16;++i)
            if(p.weapons[static_cast<std::size_t>(i)]!=0)
                out.push_back({static_cast<char>('a'+i),weaponName(i)});
    }else if(state_==State::DropArmour){
        // Index 0 is no armour and cannot be dropped.
        for(int i=1;i<6;++i)
            if(p.armour[static_cast<std::size_t>(i)]!=0)
                out.push_back({static_cast<char>('a'+i),armourName(i)});
    }
    // DropPence never opens the upper popup in the original; its amount is
    // entered only on the live command row.
    return out;
}

bool CommandSystem::beginReady(PlayerSave&,std::vector<std::string>&){
    state_=State::ReadyCategory;
    digits_.clear();
    return false;
}

bool CommandSystem::beginDrop(const LocalState& local,PlayerSave&,std::vector<std::string>& out){
    if(!local.active){ out.push_back("버리기?"); return true; }
    state_=State::DropCategory;
    digits_.clear();
    return false;
}

bool CommandSystem::handleKey(char ch,LocalState& local,PlayerSave& p,LocalSimulation& sim,
                              std::vector<std::string>& out){
    const char c=static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    switch(state_){
    case State::ReadyCategory:
        if(c=='W'){ state_=State::ReadyWeapon; return false; }
        if(c=='A'){ state_=State::ReadyArmour; return false; }
        if(c=='S'){ state_=State::ReadySpell; return false; }
        out.push_back("안 한다"); state_=State::None; return true;

    case State::ReadyWeapon:{
        const int i=itemIndex(ch,15);
        if(i<0 || p.weapons[static_cast<std::size_t>(i)]==0){
            out.push_back("안 한다"); state_=State::None; return true;
        }
        p.readyWeapon=static_cast<std::uint16_t>(i);
        out.push_back(weaponName(i)); state_=State::None; return true; }

    case State::ReadyArmour:{
        const int i=itemIndex(ch,5);
        if(i<0 || p.armour[static_cast<std::size_t>(i)]==0){
            out.push_back("안 한다"); state_=State::None; return true;
        }
        p.readyArmour=static_cast<std::uint16_t>(i);
        out.push_back(armourName(i)); state_=State::None; return true; }

    case State::ReadySpell:{
        const int i=itemIndex(ch,10);
        if(i<0 || p.spells[static_cast<std::size_t>(i)]==0){
            out.push_back("안 한다"); state_=State::None; return true;
        }
        p.readySpell=static_cast<std::uint16_t>(i);
        out.push_back(spellName(i)); state_=State::None; return true; }

    case State::DropCategory:
        if(c=='P'){ state_=State::DropPence; digits_.clear(); return false; }
        if(c=='W'){ state_=State::DropWeapon; return false; }
        if(c=='A'){ state_=State::DropArmour; return false; }
        out.push_back("안 한다"); state_=State::None; return true;

    case State::DropWeapon:{
        const int i=itemIndex(ch,15);
        if(i<1 || p.weapons[static_cast<std::size_t>(i)]==0){
            out.push_back("안 한다"); state_=State::None; return true;
        }
        out.push_back(weaponName(i));
        --p.weapons[static_cast<std::size_t>(i)];
        if(p.readyWeapon==i && p.weapons[static_cast<std::size_t>(i)]==0) p.readyWeapon=0;
        state_=State::None; return true; }

    case State::DropArmour:{
        const int i=itemIndex(ch,5);
        if(i<1 || p.armour[static_cast<std::size_t>(i)]==0){
            out.push_back("안 한다"); state_=State::None; return true;
        }
        out.push_back(armourName(i));
        --p.armour[static_cast<std::size_t>(i)];
        if(p.readyArmour==i && p.armour[static_cast<std::size_t>(i)]==0) p.readyArmour=0;
        state_=State::None; return true; }
    case State::DropPence:
        if(ch>='0'&&ch<='9'){
            if(digits_.size()<5) digits_.push_back(ch);
            return false;
        }
        if(ch=='\b'){
            if(!digits_.empty()) digits_.pop_back();
            return false;
        }
        if(ch=='\r'){
            const int amount=digits_.empty()?0:std::stoi(digits_);
            digits_.clear(); state_=State::None;
            if(amount<=0){ out.push_back("안 한다"); return true; }
            if(amount>static_cast<int>(p.coin)){ out.push_back("그만한 돈이 없습니다!"); return true; }
            p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)-amount);
            auto more=sim.dropPence(local,p,amount);
            out.insert(out.end(),more.begin(),more.end());
            return true;
        }
        return false;
    case State::None: break;
    }
    return false;
}
