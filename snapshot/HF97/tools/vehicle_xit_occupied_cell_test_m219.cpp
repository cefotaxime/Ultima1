#include "VehicleSystem.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
int gTerrain = 1; // grass everywhere: shore predicate is satisfied

[[noreturn]] void fail(const std::string& s){
    std::cerr << "M219 occupied-cell X-it FAIL: " << s << "\n";
    std::exit(1);
}
void require(bool ok,const std::string& s){ if(!ok) fail(s); }

PlayerSave riding(int transport,int physicalTile){
    PlayerSave p;
    p.x=5; p.y=5; p.transport=static_cast<std::uint16_t>(transport);
    p.worldObjects[0].w.fill(0);
    p.worldObjects[0].w[0]=static_cast<std::uint16_t>(physicalTile);
    p.worldObjects[0].w[1]=1;
    p.worldObjects[0].w[2]=p.x;
    p.worldObjects[0].w[3]=p.y;
    return p;
}

void putObject(PlayerSave& p,int index,int tile,int x,int y){
    p.worldObjects[static_cast<std::size_t>(index)].w.fill(0);
    p.worldObjects[static_cast<std::size_t>(index)].w[0]=static_cast<std::uint16_t>(tile);
    p.worldObjects[static_cast<std::size_t>(index)].w[1]=1;
    p.worldObjects[static_cast<std::size_t>(index)].w[2]=static_cast<std::uint16_t>(x);
    p.worldObjects[static_cast<std::size_t>(index)].w[3]=static_cast<std::uint16_t>(y);
}
}

int OriginalData::terrainAt(int,int) const { return gTerrain; }

int main(){
    OriginalData original;
    VehicleSystem vehicle;
    std::vector<std::string> m;

    // User-reported reproduction: Aircar attempts X-it on a Shuttle object.
    // The Shuttle must remain, Aircar must remain mounted, and no second object
    // may be appended to this coordinate.
    auto p=riding(5,16); // Aircar
    putObject(p,1,17,5,5); // Shuttle already on the same world square
    require(vehicle.exitCraft(original,p,false,m),"X-it command not consumed");
    require(!m.empty() && m.front()=="여기서 내릴 수는 없습니다!","wrong occupied-cell failure message");
    require(p.transport==5,"Aircar was dismounted onto occupied square");
    require(p.worldObjects[0].tile()==16,"player Aircar tile changed on failed X-it");
    require(p.worldObjects[1].tile()==17,"existing Shuttle was modified/removed");
    require(p.worldObjects[2].empty(),"failed X-it appended a stacked transport");

    // The rule is generic, not Shuttle-specific: another transport object blocks
    // X-it for every current craft.
    p=riding(1,11); // Horse
    putObject(p,1,12,5,5); // Cart
    m.clear();
    require(vehicle.exitCraft(original,p,false,m),"Horse occupied-cell X-it not consumed");
    require(!m.empty() && m.front()=="여기서 내릴 수는 없습니다!","Horse occupied-cell message mismatch");
    require(p.transport==1 && p.worldObjects[2].empty(),"Horse stacked with Cart");

    // Non-regression: an otherwise-valid empty grass square still allows X-it.
    p=riding(5,16);
    m.clear();
    require(vehicle.exitCraft(original,p,false,m),"normal X-it command not consumed");
    require(!m.empty() && m.front()=="내리기","normal X-it success message mismatch");
    require(p.transport==0,"normal X-it did not return to Foot");
    require(p.worldObjects[0].tile()==10,"normal X-it did not restore player tile");
    require(p.worldObjects[1].tile()==16,"normal X-it did not leave Aircar object");
    require(p.worldObjects[1].x()==5 && p.worldObjects[1].y()==5,"left Aircar object at wrong coordinate");

    std::cout << "M219 occupied-cell X-it stacking regression OK\n";
    return 0;
}
