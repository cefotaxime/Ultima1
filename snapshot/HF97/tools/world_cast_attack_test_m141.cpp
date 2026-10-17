#include "OverworldSimulation.h"
#include "OriginalData.h"
#include "LocalSimulation.h"
#include <SDL.h>
#include <iostream>
#include <string>

static void fail(const std::string& s){ std::cerr<<"M141 world Cast attack FAIL: "<<s<<"\n"; std::exit(1); }
static PlayerSave basePlayer(){
    PlayerSave p{}; p.x=50; p.y=50; p.intelligence=40; p.readyWeapon=0;
    p.worldObjects[0].w[0]=10; p.worldObjects[0].w[2]=50; p.worldObjects[0].w[3]=50; return p;
}
int main(int argc,char**argv){
    if(argc!=2) fail("usage: test GAMEDATA");
    if(SDL_Init(SDL_INIT_VIDEO)!=0) fail(SDL_GetError());
    SDL_Window* w=SDL_CreateWindow("test",0,0,64,64,0);
    SDL_Renderer* r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);
    OriginalData original; std::string err; if(!original.load(argv[1],r,err)) fail(err);
    LocalSimulation rng(original); rng.seed(1); OverworldSimulation ow(original,rng);
    {
        auto p=basePlayer(); p.worldObjects[1].w[0]=0x17; p.worldObjects[1].w[2]=52; p.worldObjects[1].w[3]=50; p.worldObjects[1].w[4]=500;
        auto out=ow.attack(p,1,0,WorldAttackMode::MagicMissile);
        if(out.empty() || out[0].find("빗")!=std::string::npos) fail("Magic Missile should hit target on cardinal ray");
        if(p.worldObjects[1].w[4]>=500) fail("Magic Missile did not apply damage");
    }
    {
        auto p=basePlayer(); p.worldObjects[1].w[0]=0x17; p.worldObjects[1].w[2]=52; p.worldObjects[1].w[3]=51; p.worldObjects[1].w[4]=500;
        auto out=ow.attack(p,1,0,WorldAttackMode::MagicMissile);
        if(out.empty() || out[0].find("빗")==std::string::npos) fail("Magic Missile must miss target off chosen cardinal ray");
    }
    {
        auto p=basePlayer(); p.worldObjects[1].w[0]=0x17; p.worldObjects[1].w[2]=51; p.worldObjects[1].w[3]=50; p.worldObjects[1].w[4]=500;
        auto out=ow.attack(p,1,0,WorldAttackMode::KillSpell);
        if(out.size()<2) fail("Kill should produce lethal hit and kill/reward rows");
        if(!p.worldObjects[1].empty()) fail("Kill did not remove target");
    }
    SDL_DestroyRenderer(r); SDL_DestroyWindow(w); SDL_Quit();
    std::cout<<"M141 overworld Magic Missile/Kill ray test OK\n";
}
