#pragma once

#include <cstdint>

// OUT.EXE file 0xB157 periodic world-tile dispatcher.
//
// Important: the original does NOT ask the graphics hardware to mirror a
// bitmap.  EGATILES.BIN already contains the alternate pose in the following
// physical tile.  Many of those B frames look like left/right-reflected poses,
// which is why the animation appears to be a horizontal flip in play.
//
// B157 selects:
//   Frigate 14/15        : wait-counter parity
//   Pirate ship 25/26    : wait-counter parity
//   monster bases 19..47 : randomRange(1,100) % 2
// Pirate ship 25 is caught by the deterministic branch before the monster range.
inline bool worldTileDeterministicPairBase(int physicalTile){
    return physicalTile==14 || physicalTile==25;
}

inline bool worldTileRandomPairBase(int physicalTile){
    return physicalTile>=19 && physicalTile<=47 && physicalTile!=25;
}

inline int worldAnimatedPhysicalTile(int baseTile,std::uint32_t waitStep,int randomBit){
    if(worldTileDeterministicPairBase(baseTile))
        return baseTile+static_cast<int>(waitStep&1u);
    if(worldTileRandomPairBase(baseTile))
        return baseTile+(randomBit&1);
    return baseTile;
}
