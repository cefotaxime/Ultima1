#include "KingSystem.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Focused KingSystem state regression. KingSystem::begin() needs only this
// OriginalData query; return the king marker directly so the test remains
// independent of renderer/data loading.
std::uint8_t OriginalData::localRawTile(int,int,int) const { return 0x3E; }

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M132 king reward popup FAIL: "<<s<<"\n";
    std::exit(1);
}
void require(bool ok,const std::string& s){ if(!ok) fail(s); }
}

int main(){
    OriginalData original;
    LocalState local;
    local.active=true;
    local.inTown=false;
    local.mapIndex=0;
    local.x=0;
    local.y=0;

    PlayerSave p;
    p.name="Avatar";
    p.strength=50;
    KingSystem king;
    int storeroomAllowance=0;
    std::vector<std::string> msg;

    // Odd king: completed quest opens a modal reward popup and grants Strength.
    local.locationId=33;
    p.kingQuest[1]=1;
    require(king.begin(original,local,p,storeroomAllowance,msg),"odd reward did not stay modal");
    require(king.active(),"odd reward popup inactive");
    require(king.stage()==KingStage::ServicePopup,"odd reward popup wrong stage");
    require(king.rewardPopup(),"odd reward popup flag missing");
    require(p.kingQuest[1]==0,"odd quest state not reset");
    require(p.strength==56,"odd Strength reward mismatch");
    require(king.popupLines().size()==2,"odd reward popup line count");
    require(king.popupLines()[0]=="오오, Avatar! 짐이 명한 임무를 완수하였구나.","odd greeting text");
    msg.clear();
    require(king.handleKey(' ',p,msg),"odd reward popup did not close on Space");
    require(!king.active(),"odd reward popup remained active");

    // Even king in quadrant 0: same modal path, red gem reward.
    local.locationId=34;
    p.kingQuest[2]=1;
    p.x=0;
    p.y=0;
    msg.clear();
    require(king.begin(original,local,p,storeroomAllowance,msg),"even reward did not stay modal");
    require(king.rewardPopup(),"even reward popup flag missing");
    require(p.kingQuest[2]==0,"even quest state not reset");
    require(p.gems[0]==1,"even red-gem reward mismatch");
    require(king.popupLines().size()==3,"even reward popup line count");
    msg.clear();
    require(king.handleKey('\r',p,msg),"even reward popup did not close on Enter");
    require(!king.active(),"even reward popup remained active");

    std::cout<<"M132 king reward popup state regression OK\n";
    return 0;
}
