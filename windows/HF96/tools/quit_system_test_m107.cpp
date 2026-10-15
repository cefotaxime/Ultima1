#include "QuitSystem.h"

#include <iostream>
#include <string>

[[noreturn]] static void fail(const std::string& s){
    std::cerr<<"M107 Quit FAIL: "<<s<<"\n";
    std::exit(1);
}

int main(){
    // Original OUT.EXE allows saving only in mode 0 (outdoor world map).
    if(!quitSaveAllowedOutdoors(false,false,false,false))
        fail("outdoor world map rejected");
    if(quitSaveAllowedOutdoors(true,false,false,false))
        fail("local town/castle incorrectly allowed");
    if(quitSaveAllowedOutdoors(false,true,false,false))
        fail("dungeon incorrectly allowed");
    if(quitSaveAllowedOutdoors(false,false,true,false))
        fail("space incorrectly allowed");
    if(quitSaveAllowedOutdoors(false,false,false,true))
        fail("Mondain mode incorrectly allowed");

    if(std::string(quitSavingText())!="게임 종료하기 - 저장중...")
        fail("saving text mismatch");
    if(std::string(quitSavedSuffix())!="저장 완료.")
        fail("saved suffix mismatch");
    if(std::string(quitIndoorsText())!="게임 종료하기 - 야외에서만 가능합니다!")
        fail("indoor text mismatch");

    std::string complete=quitSavingText();
    complete+=quitSavedSuffix();
    if(complete!="게임 종료하기 - 저장중...저장 완료.")
        fail("completed same-line text mismatch");

    if(QUIT_SAVE_VISIBLE_DELAY_MS!=500)
        fail("visible save delay is not 500 ms");

    std::cout<<"M107 Quit outdoor-only/save-delay test OK\n";
    return 0;
}