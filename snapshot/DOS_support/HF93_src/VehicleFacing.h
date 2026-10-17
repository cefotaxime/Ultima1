#pragma once

// User-requested horizontal-facing enhancement for overworld transports.
// Physical source tiles keep their original artwork and, for the Frigate,
// the original 14/15 animation pair.  Facing is applied only at final blit.
//
// Canonical tile orientation:
//   Horse (1), Cart (2)                : left
//   Raft (3), Frigate (4), Aircar (5) : right
//
// Horizontal input changes facing immediately.  Vertical input preserves the
// previous horizontal facing, including when the horizontal move was blocked.
inline bool vehicleUsesHorizontalFacing(int transport){
    return transport>=1 && transport<=5;
}

inline bool vehicleCanonicalFacesLeft(int transport){
    return transport==1 || transport==2;
}

inline bool vehicleInitialFacingLeft(int transport){
    return vehicleCanonicalFacesLeft(transport);
}

inline bool vehicleFacingLeftAfterInput(bool current,int dx,int dy){
    (void)dy;
    if(dx<0) return true;
    if(dx>0) return false;
    return current;
}

inline bool vehicleSpriteNeedsHorizontalFlip(int transport,bool facingLeft){
    if(!vehicleUsesHorizontalFacing(transport)) return false;
    return facingLeft != vehicleCanonicalFacesLeft(transport);
}
