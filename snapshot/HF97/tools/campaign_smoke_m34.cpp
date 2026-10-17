#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "OriginalData.h"
#include "LocalSimulation.h"
#include "KingSystem.h"
#include "ShopSystem.h"
#include "VehicleSystem.h"
#include "DungeonSystem.h"
#include "SpaceSystem.h"
#include "MondainSystem.h"

namespace {
[[noreturn]] void fail(const std::string& s){ std::cerr << "M34 campaign smoke FAIL: " << s << "\n"; std::exit(1); }
void require(bool v,const std::string& s){ if(!v) fail(s); }

std::pair<int,int> kingMarker(const OriginalData& o,int mapIndex){
    for(int x=0;x<OriginalData::LOCAL_W;++x)
        for(int y=0;y<OriginalData::LOCAL_H;++y)
            if(o.localRawTile(mapIndex,x,y)==0x3E) return {x,y};
    return {-1,-1};
}

int findObjectTile(const PlayerSave& p,int tile){
    for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i)
        if(!p.worldObjects[static_cast<std::size_t>(i)].empty() &&
           static_cast<int>(p.worldObjects[static_cast<std::size_t>(i)].tile())==tile) return i;
    return -1;
}

bool clearShot(const MondainState& s,int dx,int dy,int range){
    for(int k=1;k<=range;++k){
        const int x=s.playerX+dx*k, y=s.playerY+dy*k;
        if(x<0||x>=19||y<0||y>=9) return false;
        if(x==s.mondainX && y==s.mondainY) return true;
        if(s.arena[static_cast<std::size_t>(x*9+y)]!=0) return false;
    }
    return false;
}

// Find one legal step that moves toward a cardinal square from which the blaster
// can hit Mondain.  This is only a deterministic test driver; all movement and
// combat still go through MondainSystem's public gameplay commands.
std::pair<int,int> nextCombatStep(const MondainState& s,int range){
    struct Node{int x,y;};
    std::array<int,19*9> prev; prev.fill(-2);
    std::deque<Node> q;
    const int start=s.playerX*9+s.playerY;
    prev[static_cast<std::size_t>(start)]=-1;
    q.push_back({s.playerX,s.playerY});
    static constexpr int DX[4]={-1,1,0,0};
    static constexpr int DY[4]={0,0,-1,1};
    int goal=-1;
    while(!q.empty()){
        const auto n=q.front(); q.pop_front();
        // Is this a legal firing square?
        for(int d=0;d<4;++d){
            int dx=DX[d],dy=DY[d];
            for(int k=1;k<=range;++k){
                int x=n.x+dx*k,y=n.y+dy*k;
                if(x<0||x>=19||y<0||y>=9) break;
                if(x==s.mondainX&&y==s.mondainY){ goal=n.x*9+n.y; break; }
                const auto tile=s.arena[static_cast<std::size_t>(x*9+y)];
                if(tile!=0 && !(x==s.playerX&&y==s.playerY&&tile==20)) break;
            }
            if(goal>=0) break;
        }
        if(goal>=0) break;
        for(int d=0;d<4;++d){
            const int nx=n.x+DX[d],ny=n.y+DY[d];
            if(nx<0||nx>=19||ny<0||ny>=9) continue;
            if(nx==s.mondainX&&ny==s.mondainY) continue;
            const auto tile=s.arena[static_cast<std::size_t>(nx*9+ny)];
            if(tile!=0 && !(nx==s.playerX&&ny==s.playerY&&tile==20)) continue;
            const int ni=nx*9+ny;
            if(prev[static_cast<std::size_t>(ni)]!=-2) continue;
            prev[static_cast<std::size_t>(ni)]=n.x*9+n.y;
            q.push_back({nx,ny});
        }
    }
    if(goal<0) return {0,0};
    int cur=goal;
    while(prev[static_cast<std::size_t>(cur)]!=start && prev[static_cast<std::size_t>(cur)]>=0)
        cur=prev[static_cast<std::size_t>(cur)];
    if(cur==start) return {0,0};
    return {cur/9-s.playerX,cur%9-s.playerY};
}

bool secretLike(std::uint16_t f){ return f==2 || f==3; }
int desiredOrientation(int dx,int dy){
    if(dx<0) return 1;
    if(dx>0) return 2;
    if(dy<0) return 3;
    return 4;
}
void faceDungeon(DungeonSystem& ds,DungeonState& d,int want,std::vector<std::string>& msg){
    int guard=0;
    while(d.orientation!=want && guard++<4){
        msg.clear(); ds.turnRight(d,msg);
    }
    require(d.orientation==want,"could not orient dungeon player");
}

std::vector<std::pair<int,int>> dungeonPath(const DungeonSystem& ds,const DungeonState& d,int gx,int gy,bool goalMayContainMonster){
    const int W=11;
    std::array<int,121> prev; prev.fill(-1);
    std::queue<std::pair<int,int>> q;
    const int st=d.y*W+d.x; prev[static_cast<std::size_t>(st)]=st; q.push({d.x,d.y});
    static constexpr int DX[4]={-1,1,0,0}, DY[4]={0,0,-1,1};
    while(!q.empty()){
        auto [x,y]=q.front();q.pop();
        if(x==gx&&y==gy) break;
        const auto& from=ds.cell(static_cast<const DungeonState&>(d),x,y);
        for(int k=0;k<4;++k){
            int nx=x+DX[k],ny=y+DY[k]; if(nx<1||nx>9||ny<1||ny>9) continue;
            const int ni=ny*W+nx; if(prev[static_cast<std::size_t>(ni)]!=-1) continue;
            const auto& to=ds.cell(static_cast<const DungeonState&>(d),nx,ny);
            if(to.feature==1||to.feature==8) continue;
            if(secretLike(from.feature)&&secretLike(to.feature)) continue;
            // Monsters are not topology: the real player can clear them with Attack.
            // Keep them in the planned route and let walkDungeonTo kill a blocker
            // when it becomes the next step.
            (void)goalMayContainMonster;
            prev[static_cast<std::size_t>(ni)]=y*W+x; q.push({nx,ny});
        }
    }
    const int gi=gy*W+gx; if(gi<0||gi>=121||prev[static_cast<std::size_t>(gi)]==-1) return {};
    std::vector<std::pair<int,int>> out; int cur=gi;
    while(cur!=st){ out.push_back({cur%W,cur/W}); cur=prev[static_cast<std::size_t>(cur)]; }
    std::reverse(out.begin(),out.end()); return out;
}

void killDungeonMonster(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int tx,int ty,std::vector<std::string>& msg){
    int guard=0;
    while(ds.cell(static_cast<const DungeonState&>(d),tx,ty).monster!=0xFFFFu && p.hits>0 && guard++<100){
        const int dx=tx-d.x,dy=ty-d.y;
        require(std::abs(dx)+std::abs(dy)==1,"dungeon attack target not adjacent");
        faceDungeon(ds,d,desiredOrientation(dx,dy),msg);
        msg.clear(); ds.attack(d,p,msg);
    }
    require(p.hits>0,"player died killing generated dungeon monster");
    require(ds.cell(static_cast<const DungeonState&>(d),tx,ty).monster==0xFFFFu,"generated dungeon monster would not die");
}

void walkDungeonTo(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int gx,int gy,std::vector<std::string>& msg,bool goalMayContainMonster=false){
    int guard=0;
    while((d.x!=gx||d.y!=gy) && guard++<500){
        auto path=dungeonPath(ds,d,gx,gy,goalMayContainMonster);
        require(!path.empty(),"no dungeon path to requested generated cell");
        const auto [nx,ny]=path.front();
        const int dx=nx-d.x,dy=ny-d.y;
        faceDungeon(ds,d,desiredOrientation(dx,dy),msg);
        const auto& next=ds.cell(static_cast<const DungeonState&>(d),nx,ny);
        if(next.feature==2){ msg.clear(); ds.searchSecret(d,msg); require(ds.cell(static_cast<const DungeonState&>(d),nx,ny).feature==3,"secret door failed to open"); }
        if(ds.cell(static_cast<const DungeonState&>(d),nx,ny).monster!=0xFFFFu){ killDungeonMonster(ds,d,p,nx,ny,msg); continue; }
        msg.clear(); ds.forward(d,msg);
        require(d.x==nx&&d.y==ny,"normal dungeon Forward failed on planned path");
    }
    require(d.x==gx&&d.y==gy,"dungeon walk guard exhausted");
}

void descendToLevel(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int target,std::vector<std::string>& msg){
    while(d.level<target){
        int lx=-1,ly=-1;
        for(int y=1;y<=9;++y) for(int x=1;x<=9;++x) if(ds.cell(static_cast<const DungeonState&>(d),x,y).feature==6){lx=x;ly=y;}
        require(lx>=0,"down ladder missing from generated dungeon level");
        walkDungeonTo(ds,d,p,lx,ly,msg);
        faceDungeon(ds,d,4,msg);
        msg.clear(); ds.climb(d,p,msg);
    }
    require(d.level==target,"failed to descend to target level");
}

void huntQuestMonster(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int target,std::vector<std::string>& msg){
    const int slot=(target==9?2:target==14?4:target==19?6:8);
    int kills=0, idleTicks=0;
    while(p.kingQuest[static_cast<std::size_t>(slot)]==0xFFFFu && kills<250 && idleTicks<500){
        struct Candidate{int x=-1,y=-1;std::vector<std::pair<int,int>> path;bool target=false;};
        Candidate best;
        static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
        // Prefer a reachable required monster. If it is temporarily isolated by
        // other generated monsters, kill another reachable monster or let the
        // normal monster-update pass move the population before retrying.
        for(int pass=0;pass<2 && best.x<0;++pass){
            for(int ty=1;ty<=9;++ty) for(int tx=1;tx<=9;++tx){
                const auto id=ds.cell(static_cast<const DungeonState&>(d),tx,ty).monster;
                if(id==0xFFFFu) continue;
                const bool isTarget=(id==target);
                if((pass==0)!=isTarget) continue;
                for(int k=0;k<4;++k){
                    const int ax=tx+DX[k],ay=ty+DY[k]; if(ax<1||ax>9||ay<1||ay>9) continue;
                    auto cand=dungeonPath(ds,d,ax,ay,false);
                    if(ax==d.x&&ay==d.y) cand.clear();
                    const bool reachable=(ax==d.x&&ay==d.y)||!cand.empty();
                    if(!reachable) continue;
                    if(best.x<0 || cand.size()<best.path.size()) best={tx,ty,std::move(cand),isTarget};
                }
            }
        }
        if(best.x<0){
            msg.clear(); ds.updateAfterCommand(d,p,msg); ++idleTicks;
            require(p.hits>0,"player died while waiting for generated dungeon monsters to move");
            continue;
        }
        // Execute the route one square at a time; blockers are themselves cleared by
        // walkDungeonTo through the ordinary attack command.
        for(auto [nx,ny]:best.path){
            if(d.x==nx&&d.y==ny) continue;
            walkDungeonTo(ds,d,p,nx,ny,msg);
        }
        // The selected monster can move only during explicit update passes above, so
        // it remains adjacent here.
        if(ds.cell(static_cast<const DungeonState&>(d),best.x,best.y).monster!=0xFFFFu){
            killDungeonMonster(ds,d,p,best.x,best.y,msg); ++kills;
        }
    }
    require(p.kingQuest[static_cast<std::size_t>(slot)]==1,"quest monster never became reachable through generated dungeon play");
}

}

int main(int argc,char**argv){
    if(argc!=2){ std::cerr<<"usage: campaign_smoke_m33 /path/to/gamedata\n"; return 2; }
    const std::string data=argv[1];
    OriginalData original; std::string error;
    require(original.loadGameplayOracle(data,error),"gameplay oracle load: "+error);

    PlayerSave p;
    require(original.loadSave(data+"/PLAYER1.U1",p,error),"fresh PLAYER1 load: "+error);
    p.name="M34TEST";
    p.hits=9999; p.food=9999; p.coin=9999;
    p.strength=99; p.agility=99; p.stamina=99; p.wisdom=99; p.intelligence=99;
    p.readyWeapon=15; p.weapons[15]=1; p.readyArmour=5; p.armour[5]=1;
    p.experience=5000; p.spaceKills=0;
    p.kingQuest.fill(0); p.gems.fill(0);

    LocalSimulation sim(original); sim.seed(0x1234);
    DungeonSystem dungeonSystem(original,sim);
    KingSystem king;
    int storeroomAllowance=0;
    std::vector<std::string> msg;

    // Assign all four mandatory king quests at the actual even-castle world coordinates.
    static constexpr int CASTLES[4]={2,4,6,8};
    static constexpr int MONSTERS[4]={9,14,19,24};
    static constexpr int LEVELS[4]={3,5,7,9};
    const auto marker=kingMarker(original,1);
    require(marker.first>=0,"king marker 0x3E missing from castle map 1");
    for(int z=0;z<4;++z){
        const int castle=CASTLES[z];
        LocalState local; local.active=true; local.inTown=false; local.locationId=32+castle; local.mapIndex=1;
        local.x=marker.first; local.y=marker.second;
        const auto it=std::find_if(original.locations().begin(),original.locations().end(),
                                   [castle](const LocationInfo& l){return l.id==32+castle;});
        require(it!=original.locations().end(),"castle location missing from OUT oracle");
        p.x=static_cast<std::uint16_t>(it->x); p.y=static_cast<std::uint16_t>(it->y);
        msg.clear(); require(king.begin(original,local,p,storeroomAllowance,msg),"king service dialogue did not open");
        msg.clear(); require(king.handleKey('S',p,msg),"king service selection did not complete");
        require(p.kingQuest[static_cast<std::size_t>(castle)]==0xFFFFu,"king did not assign expected dungeon quest");
    }

    // Enter one real original dungeon and descend through the generated levels using
    // normal turn/search/forward/climb/attack commands.  Quest monsters are never
    // inserted by the test; if absent, ordinary kills exercise OUT.EXE's replacement
    // generator until the required member of that five-monster level band appears.
    const auto dit=std::find_if(original.locations().begin(),original.locations().end(),
                                [](const LocationInfo& l){return l.kind==LocationKind::Dungeon;});
    require(dit!=original.locations().end(),"no original dungeon location found");
    p.x=static_cast<std::uint16_t>(dit->x); p.y=static_cast<std::uint16_t>(dit->y);
    DungeonState d; msg.clear(); dungeonSystem.enter(d,p,dit->id,msg);
    for(int z=0;z<4;++z){
        descendToLevel(dungeonSystem,d,p,LEVELS[z],msg);
        huntQuestMonster(dungeonSystem,d,p,MONSTERS[z],msg);
        require(p.kingQuest[static_cast<std::size_t>(CASTLES[z])]==1,"generated dungeon did not complete mandatory quest");
    }

    // Return to each actual quest castle for the normal immediate gem reward.
    for(int z=0;z<4;++z){
        const int castle=CASTLES[z];
        LocalState local; local.active=true; local.inTown=false; local.locationId=32+castle; local.mapIndex=1;
        local.x=marker.first; local.y=marker.second;
        const auto it=std::find_if(original.locations().begin(),original.locations().end(),
                                   [castle](const LocationInfo& l){return l.id==32+castle;});
        p.x=static_cast<std::uint16_t>(it->x); p.y=static_cast<std::uint16_t>(it->y);
        msg.clear(); const bool dialogue=king.begin(original,local,p,storeroomAllowance,msg);
        require(!dialogue,"completed quest should reward immediately rather than reopen dialogue");
        require(p.gems[static_cast<std::size_t>(z)]>0,"expected quadrant gem not awarded");
    }
    require(std::all_of(p.gems.begin(),p.gems.end(),[](std::uint16_t n){return n>0;}),"four-gem gate unreachable");
    std::cout<<"PASS real generated dungeon descent/combat -> four mandatory quests -> four gems\n";

    // Buy a Shuttle through the normal town Transport shop, move to the spawned
    // world object, and Board it through the normal vehicle command.
    p.x=49; p.y=40; p.worldObjects[0].w[2]=p.x; p.worldObjects[0].w[3]=p.y;
    p.transport=0; p.worldObjects[0].w[0]=10; p.transports[6]=0;
    LocalState shopLocal; shopLocal.active=true; shopLocal.inTown=true; shopLocal.mapIndex=2;
    const auto shopMarker=kingMarker(original,2); // town map 2 also uses raw 0x3E for Transport.
    require(shopMarker.first>=0,"transport shop marker missing from town map 2");
    shopLocal.x=shopMarker.first; shopLocal.y=shopMarker.second;
    ShopSystem shop; msg.clear(); require(shop.begin(original,shopLocal,p,msg),"transport shop did not open");
    msg.clear(); require(!shop.handleKey('B',original,shopLocal,p,sim,msg),"transport shop buy stage ended unexpectedly");
    msg.clear(); require(shop.handleKey('F',original,shopLocal,p,sim,msg),"Shuttle purchase did not finish");
    require(p.transports[6]>0,"Shuttle inventory was not created by purchase");
    const int shuttleObject=findObjectTile(p,0x11);
    require(shuttleObject>0,"Shuttle purchase did not spawn a world object");
    p.x=p.worldObjects[static_cast<std::size_t>(shuttleObject)].x();
    p.y=p.worldObjects[static_cast<std::size_t>(shuttleObject)].y();
    p.worldObjects[0].w[2]=p.x; p.worldObjects[0].w[3]=p.y;
    VehicleSystem vehicles; msg.clear(); vehicles.board(p,false,msg);
    require(p.transport==6,"normal Board command did not enter purchased Shuttle");
    // The original purchase initializes these to 1000.  This is enough for 20 shots.
    require(p.shuttleStat0>=200 && p.shuttleStat1>0,"purchased Shuttle has unusable fuel/shield");
    SpaceSystem spaceSystem; SpaceState space;
    msg.clear(); spaceSystem.enter(space,p,msg);
    require(space.active,"space entry failed");
    auto& home=space.sectors[static_cast<std::size_t>(3*7+3)];
    home.enemies=20; space.frontView=true;
    for(int i=0;i<20;++i){
        space.enemySprite=6; space.enemyX=space.crosshairX; space.enemyY=space.crosshairY-1;
        msg.clear(); spaceSystem.fire(space,p,msg);
    }
    require(p.spaceKills==20,"twenty space kills were not recorded");
    require(p.experience>=7000,"Space Ace route did not reach 7000 experience");
    require(home.enemies==0,"space enemies not exhausted after twenty hits");

    // Put the shuttle into the game's real landing box and let the normal flight tick land it.
    auto& shuttle=home.craft[2]; shuttle.x=60; shuttle.y=75;
    space.frontView=false; space.velocityX=0; space.velocityY=0;
    msg.clear(); spaceSystem.updateFrame(space,p,msg);
    require(!space.active,"safe landing did not return from SPACE");
    require(p.transport==6,"landing unexpectedly discarded shuttle transport state");
    msg.clear(); vehicles.exitCraft(original,p,false,msg);
    require(p.transport==0,"X-it after safe shuttle landing failed");
    std::cout<<"PASS buy Shuttle -> Board -> 20 kills -> Space Ace -> safe landing -> X-it\n";

    // Princess rescue with the actual progression thresholds must create the Time Machine.
    const auto cit=std::find_if(original.locations().begin(),original.locations().end(),
                                [](const LocationInfo& l){return l.id==33;});
    require(cit!=original.locations().end(),"castle 33 missing");
    LocalState castle; castle.active=true; castle.inTown=false; castle.locationId=33;
    castle.mapIndex=original.localMapIndexForLocation(*cit); castle.actors=original.actorTemplate(castle.mapIndex);
    castle.tilesInitialized=true;
    for(int x=0;x<OriginalData::LOCAL_W;++x) for(int y=0;y<OriginalData::LOCAL_H;++y)
        castle.tiles[static_cast<std::size_t>(x*OriginalData::LOCAL_H+y)]=original.localRawTile(castle.mapIndex,x,y);
    int px=-1,py=-1;
    for(const auto& a:castle.actors) if(a.active()&&a.tile==0x16){px=a.x;py=a.y;break;}
    require(px>=0,"actual castle actor template has no Princess");
    // The rescue predicate itself is the exact OUT eight-neighbour test.  Use the
    // real Princess coordinate and a real passable neighbouring tile rather than
    // fabricating an actor as M33 did.
    bool placed=false;
    for(int dy=-1;dy<=1&&!placed;++dy) for(int dx=-1;dx<=1;++dx){
        if(dx==0&&dy==0) continue;
        int x=px+dx,y=py+dy;
        if(sim.passable(castle,x,y) && !sim.actorAt(castle,x,y)){castle.x=x;castle.y=y;placed=true;break;}
    }
    require(placed,"no real passable square adjacent to Princess");
    p.x=static_cast<std::uint16_t>(cit->x); p.y=static_cast<std::uint16_t>(cit->y);
    sim.seed(0x4321);
    msg=sim.rescuePrincess(castle,p);
    require(p.transports[7]>0,"princess did not grant Time Machine inventory");
    const int tmIndex=findObjectTile(p,0x12);
    require(tmIndex>0,"princess did not spawn Time Machine world object");
    const int tmx=p.worldObjects[static_cast<std::size_t>(tmIndex)].x();
    const int tmy=p.worldObjects[static_cast<std::size_t>(tmIndex)].y();
    p.x=static_cast<std::uint16_t>(tmx); p.y=static_cast<std::uint16_t>(tmy);
    p.worldObjects[0].w[2]=p.x; p.worldObjects[0].w[3]=p.y;
    msg.clear(); vehicles.board(p,false,msg);
    require(p.transport==7,"Time Machine boarding failed");
    require(findObjectTile(p,0x12)<0,"boarded Time Machine world object was not consumed");
    std::cout<<"PASS princess -> Time Machine spawn -> four-gem board gate\n";

    // Use only normal Mondain gameplay commands from entry through gem destruction and victory.
    MondainSystem mondainSystem(original); MondainState mondain;
    msg.clear(); mondainSystem.enter(mondain,p,msg);
    require(mondain.active && mondain.mondainHp==1000,"MONDAIN entry state invalid");
    // Initial corridor: walk from x=3 to x=13, next to the evil gem at x=14.
    for(int i=0;i<10;++i){
        msg.clear(); require(mondainSystem.move(mondain,1,0,msg),"blocked in initial Mondain gem corridor");
        msg.clear(); mondainSystem.updateAfterCommand(mondain,p,sim,msg);
        require(p.hits>0,"Mondain killed player before gem could be reached");
    }
    msg.clear(); require(mondainSystem.getGem(mondain,p,msg),"evil gem could not be destroyed from reachable adjacent cell");
    require(mondain.gemDestroyed && p.hits>0,"gem destruction did not leave a living player");
    msg.clear(); mondainSystem.updateAfterCommand(mondain,p,sim,msg);

    const int range=OriginalData::weaponRange(p.readyWeapon);
    require(range>0,"endgame weapon has no attack range");
    int turns=0;
    while(!mondain.victorious && p.hits>0 && turns<400){
        int attackDir=0;
        if(mondain.mondainY==mondain.playerY){
            if(mondain.mondainX<mondain.playerX && clearShot(mondain,-1,0,range)) attackDir=1;
            else if(mondain.mondainX>mondain.playerX && clearShot(mondain,1,0,range)) attackDir=2;
        }
        if(!attackDir && mondain.mondainX==mondain.playerX){
            if(mondain.mondainY<mondain.playerY && clearShot(mondain,0,-1,range)) attackDir=3;
            else if(mondain.mondainY>mondain.playerY && clearShot(mondain,0,1,range)) attackDir=4;
        }

        msg.clear();
        if(attackDir){
            require(mondainSystem.attack(mondain,p,sim,attackDir,msg),"normal Mondain attack command failed");
        }else{
            const auto step=nextCombatStep(mondain,range);
            require(step.first!=0 || step.second!=0,"no reachable firing position in Mondain arena");
            require(mondainSystem.move(mondain,step.first,step.second,msg),"combat path step blocked");
        }
        if(!mondain.victorious){
            msg.clear(); mondainSystem.updateAfterCommand(mondain,p,sim,msg);
        }
        ++turns;
    }
    require(p.hits>0,"player died during deterministic normal-command Mondain combat smoke test");
    require(mondain.victorious,"Mondain could not be defeated through normal move/attack commands after gem destruction");
    std::cout<<"PASS Time Machine -> evil gem -> normal Mondain combat -> victory ("<<turns<<" turns)\n";
    std::cout<<"M34 campaign smoke PASS\n";
    return 0;
}
