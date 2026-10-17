#include "VehicleSystem.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
int gTerrain=1;
[[noreturn]] void fail(const std::string& s){
    std::cerr << "M136 vehicle localization FAIL: " << s << "\n";
    std::exit(1);
}
void require(bool ok,const std::string& s){ if(!ok) fail(s); }
PlayerSave basePlayer(){
    PlayerSave p; p.x=5; p.y=5; p.transport=0;
    p.worldObjects[0].w.fill(0);
    p.worldObjects[0].w[0]=10; p.worldObjects[0].w[1]=1;
    p.worldObjects[0].w[2]=5; p.worldObjects[0].w[3]=5;
    return p;
}
void putObject(PlayerSave& p,int tile){
    p.worldObjects[1].w.fill(0);
    p.worldObjects[1].w[0]=static_cast<std::uint16_t>(tile);
    p.worldObjects[1].w[1]=static_cast<std::uint16_t>(gTerrain);
    p.worldObjects[1].w[2]=p.x; p.worldObjects[1].w[3]=p.y;
}
void expectBoard(VehicleSystem& v,int tile,const char* expected){
    auto p=basePlayer(); putObject(p,tile); std::vector<std::string> m;
    require(v.board(p,false,m),"board not consumed");
    require(!m.empty() && m.front()==expected,std::string("board text: ")+expected);
}
}
int OriginalData::terrainAt(int,int) const { return gTerrain; }

int main(){
    VehicleSystem v; std::vector<std::string> m;
    expectBoard(v,11,"말을 탑니다!");
    expectBoard(v,12,"마차를 탑니다!");
    expectBoard(v,13,"뗏목을 탑니다!");
    expectBoard(v,14,"승선합니다!");
    expectBoard(v,17,"우주 왕복선에 탑승합니다!");

    auto p=basePlayer();
    require(v.board(p,false,m) && m.front()=="무엇을 탑니까?","Nothing to Board localization");
    p=basePlayer(); p.transport=1; m.clear();
    require(v.board(p,false,m) && m.front()=="먼저 내려야 합니다!","X-it first localization");
    p=basePlayer(); m.clear();
    require(v.exitCraft(OriginalData{},p,false,m) && m.front()=="무엇에서 내립니까?","X-it what localization");

    p=basePlayer(); m.clear(); gTerrain=0;
    require(!v.mayMove(OriginalData{},p,6,5,m) && m.front()=="물 위를 걸을 수 없습니다!","water localization");
    p=basePlayer(); m.clear(); gTerrain=3;
    require(!v.mayMove(OriginalData{},p,6,5,m) && m.front()=="산을 통과할 수 없습니다!","mountain localization");
    p=basePlayer(); p.transport=5; m.clear(); gTerrain=2;
    require(!v.mayMove(OriginalData{},p,6,5,m) && m.front()=="비행선은 숲을 통과할 수 없습니다!","aircar localization");
    p=basePlayer(); p.transport=4; m.clear(); gTerrain=1;
    require(!v.mayMove(OriginalData{},p,6,5,m) && m.front()=="범선은 바다를 좋아합니다!","frigate localization");
    p=basePlayer(); p.transport=3; m.clear(); gTerrain=1;
    require(!v.mayMove(OriginalData{},p,6,5,m) && m.front()=="뗏목은 바다를 좋아합니다!","raft localization");

    std::cout << "M136 vehicle localization test OK\n";
}
