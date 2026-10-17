#include "FrigateDirection.h"
#include <cstdlib>
#include <iostream>

static void require(bool ok,const char* what){
    if(!ok){ std::cerr << "M137 Frigate input-facing FAIL: " << what << "\n"; std::exit(1); }
}

int main(){
    bool left=false; // canonical/right-facing

    // Facing changes at input time; the caller may reject movement afterwards.
    left=frigateFacingLeftAfterInput(left,-1,0);
    require(left,"blocked west attempt must still turn the Frigate left");

    // Vertical inputs never alter the horizontal facing.
    left=frigateFacingLeftAfterInput(left,0,-1);
    require(left,"north must preserve left-facing state");
    left=frigateFacingLeftAfterInput(left,0,1);
    require(left,"south must preserve left-facing state");

    left=frigateFacingLeftAfterInput(left,1,0);
    require(!left,"blocked east attempt must still turn the Frigate right");

    std::cout << "M137 Frigate input-facing state test OK\n";
}
