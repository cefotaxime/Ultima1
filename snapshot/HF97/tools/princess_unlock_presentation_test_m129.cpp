#include "KoreanText.h"
#include "LocalSimulation.h"
#include "OriginalData.h"
#include "PrincessRescuePresentation.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M129 Princess/Unlock presentation FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    // Princess transliterations; M130 adds the previously missing Julia.
    if(공주_이름_한글("Cassandra")!="카산드라") fail("Cassandra");
    if(공주_이름_한글("Marsha")!="마샤") fail("Marsha");
    if(공주_이름_한글("Donna")!="돈나") fail("Donna");
    if(공주_이름_한글("Kristen")!="크리스턴") fail("Kristen");
    if(공주_이름_한글("Beth")!="베스") fail("Beth");
    if(공주_이름_한글("Lori")!="로리") fail("Lori");
    if(공주_이름_한글("Dianne")!="다이앤") fail("Dianne");
    if(공주_이름_한글("Julia")!="줄리아") fail("Julia");

    // Exact castle Unlock result translations.
    OriginalData original;
    LocalSimulation sim(original);
    PlayerSave player;
    LocalState local;
    local.active=true;
    local.inTown=false;
    local.tilesInitialized=true;
    local.tiles.fill(1);
    local.x=4;
    local.y=4;
    local.tiles[static_cast<std::size_t>(local.x*OriginalData::LOCAL_H+local.y)]=0x3C;
    local.foundKey=true;
    local.correctKeyTile=0x3D;

    auto out=sim.unlock(local,player);
    if(out.size()!=2 ||
       out[0]!="무슨 자물쇠를 엽니까?" ||
       out[1]!="올바른 열쇠가 아닙니다!")
        fail("wrong-key translation");

    local.correctKeyTile=0x3C;
    out=sim.unlock(local,player);
    if(out.size()!=2 ||
       out[0]!="무슨 자물쇠를 엽니까?" ||
       out[1]!="문이 열렸습니다!")
        fail("door-open translation");

    // Popup must become dismissible only after its first actual present and
    // the original five-DOS-tick minimum hold.
    PrincessRescuePresentation popup;
    constexpr std::uint32_t HOLD=275;
    popup.begin();
    if(!popup.active() || !popup.awaitingFirstPresent()) fail("popup begin state");
    if(popup.canDismiss(99999,HOLD)) fail("dismiss before first present");
    popup.afterPresent(1000);
    if(!popup.waitingForDismiss()) fail("popup did not enter wait after present");
    if(popup.canDismiss(1274,HOLD)) fail("popup dismissed one ms early");
    if(!popup.canDismiss(1275,HOLD)) fail("popup did not dismiss at hold boundary");
    popup.clear();
    if(popup.active()) fail("popup clear");

    std::cout<<"M129 Princess popup/name + Unlock translation test OK\n";
    return 0;
}
