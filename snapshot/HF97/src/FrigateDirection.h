#pragma once
#include "VehicleFacing.h"

// Historical M136/M137 compatibility wrapper.  The Frigate uses the same
// persistent horizontal-facing state machine now shared by transports 1..5.
inline bool frigateFacingLeftAfterInput(bool current,int dx,int dy){
    return vehicleFacingLeftAfterInput(current,dx,dy);
}