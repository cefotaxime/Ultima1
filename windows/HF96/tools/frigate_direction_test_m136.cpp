#include "FrigateDirection.h"
#include <cstdlib>
#include <iostream>

static void require(bool ok,const char* what){
    if(!ok){ std::cerr << "M136 Frigate direction FAIL: " << what << "\n"; std::exit(1); }
}

int main(){
    bool left=false;
    left=frigateFacingLeftAfterInput(left,1,0);
    require(!left,"east must use canonical right-facing sprite");
    left=frigateFacingLeftAfterInput(left,-1,0);
    require(left,"west must mirror sprite");
    left=frigateFacingLeftAfterInput(left,0,-1);
    require(left,"north must preserve west-facing state");
    left=frigateFacingLeftAfterInput(left,0,1);
    require(left,"south must preserve west-facing state");
    left=frigateFacingLeftAfterInput(left,1,0);
    require(!left,"east must restore canonical orientation");
    left=frigateFacingLeftAfterInput(left,0,1);
    require(!left,"south must preserve east-facing state");
    std::cout << "M136 Frigate direction state test OK\n";
}