#include "VisiblePresentationHold.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M126 visible presentation hold FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    VisiblePresentationHold h;

    // Scheduling itself must not start the delay clock.
    h.begin();
    if(!h.pending()) fail("begin did not set pending");
    if(h.started()) fail("clock started before any frame was presented");
    if(h.ready(5000,1000)) fail("hold became ready before first present");

    // The first presented frame defines t=0 of the user-visible hold.
    h.afterPresent(5000);
    if(!h.started() || h.startedMs()!=5000) fail("first present did not arm clock");
    if(h.ready(5999,1000)) fail("1-second hold expired at 999 ms");
    if(!h.ready(6000,1000)) fail("1-second hold did not expire at 1000 ms");

    // Later frames must not restart/extend the timer.
    h.afterPresent(5500);
    if(h.startedMs()!=5000) fail("later present restarted timer");

    // clear/begin creates a fresh hold.
    h.clear();
    if(h.pending() || h.started()) fail("clear did not reset state");
    h.begin();
    if(h.ready(123456,1000)) fail("new hold inherited old timer");
    h.afterPresent(9000);
    if(!h.ready(10000,1000)) fail("second hold exact delay failed");

    // Unsigned subtraction keeps the normal SDL_GetTicks() wraparound behavior.
    h.clear();
    h.begin();
    h.afterPresent(0xFFFFFFF0u);
    if(h.ready(0x000003D7u,1000)) fail("wraparound hold expired one ms early");
    if(!h.ready(0x000003D8u,1000)) fail("wraparound hold did not expire at 1000 ms");

    std::cout<<"M126 visible-after-present 1-second hold test OK\n";
    return 0;
}
