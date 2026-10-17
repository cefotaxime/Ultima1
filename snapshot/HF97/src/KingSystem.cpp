#include "KingSystem.h"
#include "KoreanText.h"
#include <cctype>
#include <cstdint>

#include <algorithm>
namespace {
char upper(char c){ return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }

const char* LANDMARK_TARGET[9]={
    "", "잊힌 영혼의 무덤", "", "남쪽 표지판", "",
    "오지만디아스의 기둥", "", "지식의 탑", ""
};
const char* MONSTER_TARGET[9]={
    "", "", "젤라틴 큐브", "", "송장 벌레", "", "리치", "", "발론"
};
}

int KingSystem::quadrant(const PlayerSave& p){
    // OUT.EXE 0xBB56: 2*(Y>77) + (X>83).
    return (p.y>77?2:0) + (p.x>83?1:0);
}

bool KingSystem::begin(const OriginalData& original,const LocalState& local,PlayerSave& p,
                       int& storeroomAllowance,std::vector<std::string>& msg){
    if(!local.active || local.inTown){ msg.push_back("거래하기?"); return false; }
    msg.push_back("국왕과 거래하기");
    if(original.localRawTile(local.mapIndex,local.x,local.y)!=0x3E){ msg.push_back("국왕이 여기 없습니다!"); return false; }
    if(local.alert){ msg.push_back("왕이 당신의 알현을 거절합니다!"); return false; }
    castleIndex_=local.locationId-32;
    if(castleIndex_<1 || castleIndex_>8){ msg.push_back("국왕이 여기 없습니다!"); close(); return false; }
    if(p.kingQuest[static_cast<std::size_t>(castleIndex_)]==1){
        rewardCompleted(p,storeroomAllowance,msg);
        rewardPopup_=true;
        stage_=KingStage::ServicePopup;
        return true;
    }
    rewardPopup_=false;
    stage_=KingStage::Choice;
    number_.clear();
    return true;
}

void KingSystem::rewardCompleted(PlayerSave& p,int& storeroomAllowance,std::vector<std::string>& msg){
    (void)msg;
    p.kingQuest[static_cast<std::size_t>(castleIndex_)]=0;
    popupLines_.clear();
    popupLines_.push_back("오오, "+p.name+"! 짐이 명한 임무를 완수하였구나.");

    if(castleIndex_&1){
        int points=(99-static_cast<int>(p.strength))/8;
        if(points==0 && p.strength!=99) points=1;
        popupLines_.push_back("보답으로 자네의 힘을 "+std::to_string(points)+"만큼 높여 주겠네.");
        p.strength=static_cast<std::uint16_t>(static_cast<unsigned>(p.strength)+static_cast<unsigned>(points));
        return;
    }

    switch(quadrant(p)){
        case 0:
            popupLines_.push_back("보답으로 알려 주지. 타임머신을 작동하려면 네 개의 보석이 필요하다네!");
            popupLines_.push_back("국왕이 당신에게 붉은색 보석을 하사합니다.");
            ++p.gems[0];
            break;
        case 1:
            popupLines_.push_back("보답으로 알려 주지. 자네가 승리하려면 반드시 타임머신을 사용해야 하네!");
            popupLines_.push_back("국왕이 당신에게 녹색 보석을 하사합니다!");
            ++p.gems[1];
            break;
        case 2:
            popupLines_.push_back("보답으로 알려 주지. 우주 영웅에 등극하면 공주가 시간을 넘나들도록 도울 것이네!");
            popupLines_.push_back("국왕이 당신에게 푸른색 보석을 하사합니다.");
            ++p.gems[2];
            break;
        case 3:
            popupLines_.push_back("보답으로 짐의 저장고에서 9개의 물건을 가져가도록 윤허하겠네. 9개 뿐이네!");
            popupLines_.push_back("국왕이 또한 당신에게 흰색 보석을 하사합니다!");
            ++p.gems[3];
            // OUT.EXE 0x7A92: DS:8BA4 = 9.  This is runtime-only and is
            // consumed by Get at castle storeroom counters.
            storeroomAllowance=9;
            break;
    }
}

bool KingSystem::assignServiceQuest(PlayerSave& p,std::vector<std::string>& msg){
    auto& state=p.kingQuest[static_cast<std::size_t>(castleIndex_)];
    if(state==0xFFFFu){
        msg.push_back("짐은 이미 네게 임무를 내렸느니라!");
        return false;
    }

    state=0xFFFFu;
    rewardPopup_=false;
    popupLines_.clear();
    if(castleIndex_&1){
        popupLines_.push_back("가서 아래 유적을 찾으라");
        popupLines_.push_back(LANDMARK_TARGET[castleIndex_]);
    }else{
        popupLines_.push_back("이제 가서 아래 괴수를 처치하라");
        popupLines_.push_back(MONSTER_TARGET[castleIndex_]);
    }
    popupLines_.push_back("임무를 완수한 후에 돌아오너라!");
    stage_=KingStage::ServicePopup;
    return true;
}

bool KingSystem::handleKey(char key,PlayerSave& p,std::vector<std::string>& msg){
    if(stage_==KingStage::Idle) return false;
    if(stage_==KingStage::Choice){
        const char u=upper(key);
        if(u=='S'){
            if(assignServiceQuest(p,msg)) return false;
            close();
            return true;
        }
        if(u=='P'){
            msg.push_back("공물");
            stage_=KingStage::PenceAmount;
            number_.clear();
            return false;
        }
        msg.push_back("안 한다");
        close();
        return true;
    }
    if(stage_==KingStage::PenceAmount){
        if(key>='0'&&key<='9'){ if(number_.size()<5) number_.push_back(key); return false; }
        if(key=='\b'){ if(!number_.empty()) number_.pop_back(); return false; }
        if(key=='\r'||key=='\n'){
            const unsigned amount=number_.empty()?0u:static_cast<unsigned>(std::stoul(number_));
            if(amount==0){ msg.push_back("바치지 않는다"); close(); return true; }
            if(amount>p.coin){ msg.push_back("그만한 돈이 없습니다!"); close(); return true; }
            p.coin=static_cast<std::uint16_t>(p.coin-static_cast<std::uint16_t>(amount));
            // OUT.EXE 0x7C8B..0x7C99: 16-bit IMUL by 3, then signed divide by 2.
            const std::int16_t low=static_cast<std::int16_t>(static_cast<std::uint16_t>(amount*3u));
            const int gain=static_cast<int>(low)/2;
            msg.push_back("보답으로 네게 생명력 "+std::to_string(gain)+"를 내리노라");
            p.hits=static_cast<std::uint16_t>(
                std::min(9999,static_cast<int>(p.hits)+gain));
            close();
            return true;
        }
    }
    if(stage_==KingStage::ServicePopup){
        if(key==' ' || key=='\r' || key=='\n'){
            close();
            return true;
        }
        return false;
    }
    return false;
}

bool KingSystem::cancel(std::vector<std::string>& msg){
    if(!active()) return false;
    msg.push_back("안 한다");
    close();
    return true;
}

std::string KingSystem::inputLine() const{
    if(stage_==KingStage::Choice) return "공물(p)을 바칩니까, 일(s)을 합니까: ";
    if(stage_==KingStage::PenceAmount) return std::string("얼마를 바칩니까? ")+number_;
    if(stage_==KingStage::ServicePopup) return "스페이스바를 눌러 계속하세요: ";
    return {};
}

bool KingSystem::visitLandmark(int locationId,PlayerSave& p,int& lastLandmarkIndex,
                               std::vector<std::string>& msg,int* rewardSfxPending){
    if(rewardSfxPending) *rewardSfxPending=-1;
    if(locationId<41||locationId>48) return false;
    const int index=locationId-41;

    // OUT.EXE 0x50A2 renders one landmark-specific message block.
    switch(index){
        case 0:
            msg.push_back("강력한 마법의 기운이 당신을 에워쌉니다!");
            break;
        case 1:
            msg.push_back("누군가가 \"Turis-scientia-magnopere\"라고 말합니다.");
            msg.push_back("(역자주: 지혜의 탑의 위업)");
            break;
        case 2:
            msg.push_back("표지판에는 \"Fortes fortuna adiuvat!\"라고 써 있습니다.");
            msg.push_back("(역자주: 행운은 용자를 돕는다)");
            break;
        case 3:
            msg.push_back("대좌 위에 글자가 나타납니다.");
            msg.push_back("\"짐은 왕 중의 왕 오지만디아스다. 강한 이여, 짐의 위업을 보고 절망하라!\"");
            msg.push_back("그 외에는 아무것도 없습니다. 당신은 기묘한 기운을 느낍니다!");
            break;
        case 4:
            msg.push_back("표지판에는 \"Ultima thule!\"라고 써 있습니다.");
            msg.push_back("(역자주: 미지의 땅)");
            msg.push_back("하늘이 어두워지고, 강력한 마법이 당신을 집어삼킵니다!");
            break;
        case 5:
            msg.push_back("표지판에는 \"Omnia mutantur!\"라고 써 있습니다.");
            msg.push_back("(역자주: 만물은 변화한다)");
            break;
        case 6:
            msg.push_back("표지판에는 \"동쪽으로 가려면 동쪽으로 가라!\"라고 써 있습니다.");
            break;
        case 7:
            msg.push_back("묘비에 \"Vae Victis\"라고 새겨져 있습니다.");
            msg.push_back("(역자주: 패배한 자에게 재앙 있으리라)");
            break;
    }

    // OUT.EXE 0x5285: odd-index landmarks can complete the matching King's
    // service quest before the normal landmark reward is processed.
    if(index&1){
        const int castleIndex=8-index;
        auto& q=p.kingQuest[static_cast<std::size_t>(castleIndex)];
        if(q==0xFFFFu){
            q=1;
            msg.push_back("임무를 완료하였습니다!");
        }
    }

    // OUT.EXE DS:3B20 suppresses the reward when entering the same landmark
    // repeatedly without visiting a different one in between.
    if(lastLandmarkIndex==index){
        if(index!=6){
            msg.push_back("흐음...아무런 효과도 없습니다!");
            if(rewardSfxPending) *rewardSfxPending=6;
        }
        return true;
    }
    lastLandmarkIndex=index;

    // OUT.EXE DS:3708: reward selector per landmark index.
    static constexpr int REWARD_TYPE[8]={2,6,0,5,3,4,0,3};

    if(REWARD_TYPE[index]==0){
        if(index!=2) return true; // Eastern Sign Post: text only.

        // Pillars of the Argonauts: grant the lowest-index missing weapon.
        int weapon=-1;
        for(int i=1;i<=15;++i){
            if(p.weapons[static_cast<std::size_t>(i)]==0){ weapon=i; break; }
        }
        if(weapon<0) return true;
        ++p.weapons[static_cast<std::size_t>(weapon)];
        msg.push_back(조사_처리기("당신은 ["+std::string(무기_이름_한글(weapon))+"]@을 발견합니다."));
        if(rewardSfxPending) *rewardSfxPending=5;
        return true;
    }

    std::uint16_t* stat=nullptr;
    const char* statName="";
    switch(REWARD_TYPE[index]){
        case 2: stat=&p.agility;      statName="민첩"; break;
        case 3: stat=&p.stamina;      statName="지구력"; break;
        case 4: stat=&p.charisma;     statName="매력"; break;
        case 5: stat=&p.wisdom;       statName="지혜"; break;
        case 6: stat=&p.intelligence; statName="지능"; break;
        default: break;
    }
    if(!stat) return true;
    if(*stat==99){
        msg.push_back("흐음...아무런 효과도 없습니다!");
        if(rewardSfxPending) *rewardSfxPending=6;
        return true;
    }

    int gain=(static_cast<int>(*stat)+8)/10;
    if(static_cast<int>(*stat)+gain>99) gain=99-static_cast<int>(*stat);
    *stat=static_cast<std::uint16_t>(static_cast<int>(*stat)+gain);

    if(REWARD_TYPE[index]==2)
        msg.push_back("당신의 민첩이 "+std::to_string(gain)+"만큼 증가합니다.");
    else if(REWARD_TYPE[index]==3)
        msg.push_back("당신의 지구력이 "+std::to_string(gain)+"만큼 증가합니다.");
    else if(REWARD_TYPE[index]==4)
        msg.push_back("당신의 매력이 "+std::to_string(gain)+"만큼 증가합니다.");
    else if(REWARD_TYPE[index]==5)
        msg.push_back("당신의 지혜가 "+std::to_string(gain)+"만큼 증가합니다.");
    else if(REWARD_TYPE[index]==6)
        msg.push_back("당신의 지능이 "+std::to_string(gain)+"만큼 증가합니다.");

    if(rewardSfxPending) *rewardSfxPending=5;
    return true;
}