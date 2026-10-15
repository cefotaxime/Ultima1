#include "VehicleSystem.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
int gTerrain = 0;

[[noreturn]] void fail(const std::string& s){
    std::cerr << "M135 vehicle board access FAIL: " << s << "\n";
    std::exit(1);
}

void require(bool ok,const std::string& s){ if(!ok) fail(s); }

PlayerSave basePlayer(){
    PlayerSave p;
    p.transport=0;
    p.x=5; p.y=5;
    p.worldObjects[0].w.fill(0);
    p.worldObjects[0].w[0]=10;
    p.worldObjects[0].w[1]=1;
    p.worldObjects[0].w[2]=5;
    p.worldObjects[0].w[3]=5;
    return p;
}

void putObject(PlayerSave& p,int tile,int x,int y){
    p.worldObjects[1].w.fill(0);
    p.worldObjects[1].w[0]=static_cast<std::uint16_t>(tile);
    p.worldObjects[1].w[1]=0;
    p.worldObjects[1].w[2]=static_cast<std::uint16_t>(x);
    p.worldObjects[1].w[3]=static_cast<std::uint16_t>(y);
}
}

// Focused link stub: this regression only needs to control the raw terrain
// returned underneath a live world object.  No SDL/data-file loading is used.
int OriginalData::terrainAt(int,int) const { return gTerrain; }

int main(){
    OriginalData original;
    VehicleSystem vehicle;
    std::vector<std::string> msg;

    // Ordinary water is still forbidden on foot.
    auto p=basePlayer();
    gTerrain=0;
    require(!vehicle.mayMove(original,p,6,5,msg),"bare water became walkable");
    require(!msg.empty(),"bare-water failure lost its message");

    // OUT.EXE's object-aware cell query substitutes Frigate tile 14 for the
    // water below it.  The player must therefore be able to step onto it.
    p=basePlayer(); msg.clear(); putObject(p,14,6,5);
    require(vehicle.mayMove(original,p,6,5,msg),"cannot step onto Frigate over water");
    require(msg.empty(),"Frigate access emitted a terrain failure");

    // Then Board must consume that object and turn it into the live transport.
    p.x=6; p.y=5; p.worldObjects[0].w[2]=6; p.worldObjects[0].w[3]=5;
    msg.clear();
    require(vehicle.board(p,false,msg),"Board command not consumed");
    require(p.transport==4,"Frigate did not become current transport");
    require(p.worldObjects[0].tile()==14,"player physical tile did not become Frigate");
    require(p.worldObjects[1].empty(),"Frigate world object was not removed");

    // Raft uses the same original access rule.
    p=basePlayer(); msg.clear(); putObject(p,13,6,5);
    require(vehicle.mayMove(original,p,6,5,msg),"cannot step onto Raft over water");

    // A roaming monster object remains an obstruction even over otherwise
    // valid terrain; the object-aware query must not turn into a generic pass.
    p=basePlayer(); msg.clear(); putObject(p,19,6,5); gTerrain=1;
    require(!vehicle.mayMove(original,p,6,5,msg),"monster object became walkable");

    // Once boarded, Frigate movement is still water-only.
    p=basePlayer(); p.transport=4; p.worldObjects[0].w[0]=14;
    p.worldObjects[1].w.fill(0);
    msg.clear(); gTerrain=0;
    require(vehicle.mayMove(original,p,6,5,msg),"boarded Frigate cannot sail on water");
    msg.clear(); gTerrain=1;
    require(!vehicle.mayMove(original,p,6,5,msg),"boarded Frigate can sail on land");

    std::cout << "M135 vehicle object-aware movement / Frigate boarding test OK\n";
    return 0;
}