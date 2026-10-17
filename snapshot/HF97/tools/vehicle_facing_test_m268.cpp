#include "VehicleFacing.h"
#include "WorldTileAnimation.h"
#include <cstdlib>
#include <iostream>

static void req(bool ok,const char* msg){
    if(!ok){ std::cerr << "M268 vehicle facing FAIL: " << msg << "\n"; std::exit(1); }
}

int main(){
    // Canonical artwork directions.
    req(vehicleInitialFacingLeft(1),"Horse source art must face left");
    req(vehicleInitialFacingLeft(2),"Cart source art must face left");
    req(!vehicleInitialFacingLeft(3),"Raft source art must face right");
    req(!vehicleInitialFacingLeft(4),"Frigate source art must face right");
    req(!vehicleInitialFacingLeft(5),"Aircar source art must face right");

    // Horizontal input sets facing; vertical preserves it.
    bool left=vehicleInitialFacingLeft(1);
    left=vehicleFacingLeftAfterInput(left,1,0);
    req(!left,"Horse east must face right");
    req(vehicleSpriteNeedsHorizontalFlip(1,left),"Horse east must mirror canonical left art");
    left=vehicleFacingLeftAfterInput(left,0,-1);
    req(!left && vehicleSpriteNeedsHorizontalFlip(1,left),"Horse north must preserve right facing");
    left=vehicleFacingLeftAfterInput(left,-1,0);
    req(left && !vehicleSpriteNeedsHorizontalFlip(1,left),"Horse west must restore canonical art");

    left=vehicleInitialFacingLeft(3);
    left=vehicleFacingLeftAfterInput(left,-1,0);
    req(left && vehicleSpriteNeedsHorizontalFlip(3,left),"Raft west must mirror canonical right art");
    left=vehicleFacingLeftAfterInput(left,0,1);
    req(left && vehicleSpriteNeedsHorizontalFlip(3,left),"Raft south must preserve left facing");
    left=vehicleFacingLeftAfterInput(left,1,0);
    req(!left && !vehicleSpriteNeedsHorizontalFlip(3,left),"Raft east must restore canonical art");

    // Frigate animation frame selection remains independent of facing.
    req(worldAnimatedPhysicalTile(14,0,0)==14,"Frigate even step must use tile 14");
    req(worldAnimatedPhysicalTile(14,1,0)==15,"Frigate odd step must use tile 15");
    req(vehicleSpriteNeedsHorizontalFlip(4,true),"left-facing Frigate must mirror either frame");
    req(!vehicleSpriteNeedsHorizontalFlip(4,false),"right-facing Frigate must keep either frame canonical");

    // Shuttle and Time Machine remain outside this enhancement.
    req(!vehicleUsesHorizontalFacing(6),"Shuttle must not use horizontal-facing enhancement");
    req(!vehicleUsesHorizontalFacing(7),"Time Machine must not use horizontal-facing enhancement");

    std::cout << "M268 vehicle facing / Frigate animation test OK\n";
}
