#include "LocalActorDialoguePresentation.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr << "M127 local actor dialogue presentation FAIL: " << s << "\n";
    std::exit(1);
}
}

int main(){
    LocalActorDialoguePresentation p;
    if(p.active()) fail("fresh state active");

    p.begin();
    if(!p.active()) fail("begin not active");
    if(!p.awaitingImmediatePresent()) fail("begin not awaiting immediate frame");
    if(p.awaitingDialoguePresent()) fail("begin already awaiting dialogue frame");

    p.dialogueQueued();
    if(!p.active()) fail("dialogueQueued lost active state");
    if(p.awaitingImmediatePresent()) fail("dialogueQueued still awaiting immediate frame");
    if(!p.awaitingDialoguePresent()) fail("dialogueQueued not awaiting dialogue frame");

    p.finish();
    if(p.active()) fail("finish did not clear state");

    p.begin();
    p.clear();
    if(p.active()) fail("clear did not reset state");

    std::cout << "M127 two-sided dialogue presentation state test OK\n";
    return 0;
}
