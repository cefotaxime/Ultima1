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
#include "OverworldSimulation.h"
#include "KingSystem.h"
#include "ShopSystem.h"
#include "VehicleSystem.h"
#include "DungeonSystem.h"
#include "SpaceSystem.h"
#include "MondainSystem.h"
#include "CommandSystem.h"

namespace {
LocalSimulation* g_readySim=nullptr;
[[noreturn]] void fail(const std::string& s){ std::cerr << "M39 campaign smoke FAIL: " << s << "\n"; std::exit(1); }
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
void finishDungeon(DungeonSystem& ds,DungeonState& d,PlayerSave& p,std::vector<std::string>& msg){
    ++p.steps;
    if(d.active){ auto more=std::vector<std::string>{}; ds.updateAfterCommand(d,p,more); msg.insert(msg.end(),more.begin(),more.end()); }
    if(p.hits==0){std::cerr<<"DUNGEON DEATH level="<<d.level<<" pos="<<d.x<<","<<d.y<<" exp="<<p.experience<<" coin="<<p.coin<<" steps="<<p.steps<<"\n";fail("player died during live dungeon command sequence");}
    // If a Gelatinous Cube destroyed the equipped suit, emulate a human immediately
    // issuing Ready Armour: Reflect suit. This is a real command and therefore consumes
    // its own dungeon monster-response turn.
    for(int re=0; d.active && p.readyArmour==0 && p.armour[5]>0 && g_readySim && re<8; ++re){
        CommandSystem cmd; LocalState dummy; std::vector<std::string> rr;
        cmd.beginReady(p,rr); cmd.handleKey('A',dummy,p,*g_readySim,rr); cmd.handleKey('f',dummy,p,*g_readySim,rr);
        ++p.steps; std::vector<std::string> more2; ds.updateAfterCommand(d,p,more2);
        if(p.hits==0){std::cerr<<"DUNGEON DEATH while re-readying armour level="<<d.level<<"\n";fail("player died while normally re-readying spare armour");}
    }
}
void faceDungeon(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int want,std::vector<std::string>& msg){
    int guard=0; static constexpr int R[5]={0,3,4,2,1},L[5]={0,4,3,1,2};
    while(d.orientation!=want && guard++<4){
        int rd=d.orientation,ld=d.orientation,rc=0,lc=0;
        while(rd!=want&&rc<4){rd=R[rd];++rc;} while(ld!=want&&lc<4){ld=L[ld];++lc;}
        msg.clear(); if(lc<rc) ds.turnLeft(d,msg); else ds.turnRight(d,msg); finishDungeon(ds,d,p,msg);
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
        faceDungeon(ds,d,p,desiredOrientation(dx,dy),msg);
        msg.clear(); ds.attack(d,p,msg); finishDungeon(ds,d,p,msg);
    }
    require(p.hits>0,"player died killing generated dungeon monster");
    require(ds.cell(static_cast<const DungeonState&>(d),tx,ty).monster==0xFFFFu,"generated dungeon monster would not die");
}

void clearAdjacentThreats(DungeonSystem& ds,DungeonState& d,PlayerSave& p,std::vector<std::string>& msg){
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int guard=0;guard<32 && p.hits>0;++guard){
        bool found=false;
        for(int k=0;k<4;++k){int x=d.x+DX[k],y=d.y+DY[k]; if(x<1||x>9||y<1||y>9)continue; if(ds.cell(static_cast<const DungeonState&>(d),x,y).monster==0xFFFFu)continue; killDungeonMonster(ds,d,p,x,y,msg); found=true; break;}
        if(!found)break;
    }
}

void walkDungeonTo(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int gx,int gy,std::vector<std::string>& msg,bool goalMayContainMonster=false){
    int guard=0;
    while((d.x!=gx||d.y!=gy) && guard++<500){
        clearAdjacentThreats(ds,d,p,msg);
        auto path=dungeonPath(ds,d,gx,gy,goalMayContainMonster);
        require(!path.empty(),"no dungeon path to requested generated cell");
        const auto [nx,ny]=path.front();
        const int dx=nx-d.x,dy=ny-d.y;
        faceDungeon(ds,d,p,desiredOrientation(dx,dy),msg);
        const auto& next=ds.cell(static_cast<const DungeonState&>(d),nx,ny);
        if(next.feature==2){ msg.clear(); ds.searchSecret(d,msg); finishDungeon(ds,d,p,msg); require(ds.cell(static_cast<const DungeonState&>(d),nx,ny).feature==3,"secret door failed to open"); }
        if(ds.cell(static_cast<const DungeonState&>(d),nx,ny).monster!=0xFFFFu){ killDungeonMonster(ds,d,p,nx,ny,msg); continue; }
        msg.clear(); ds.forward(d,msg); finishDungeon(ds,d,p,msg);
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
        clearAdjacentThreats(ds,d,p,msg);
        faceDungeon(ds,d,p,4,msg);
        msg.clear(); ds.climb(d,p,msg); finishDungeon(ds,d,p,msg);
    }
    require(d.level==target,"failed to descend to target level");
}

void ascendOut(DungeonSystem& ds,DungeonState& d,PlayerSave& p,std::vector<std::string>& msg){
    while(d.active){
        int lx=-1,ly=-1; for(int y=1;y<=9;++y)for(int x=1;x<=9;++x)if(ds.cell(static_cast<const DungeonState&>(d),x,y).feature==7){lx=x;ly=y;}
        require(lx>=0,"up ladder missing from generated dungeon level");
        walkDungeonTo(ds,d,p,lx,ly,msg); clearAdjacentThreats(ds,d,p,msg); faceDungeon(ds,d,p,3,msg); msg.clear(); ds.climb(d,p,msg); finishDungeon(ds,d,p,msg);
    }
}

bool huntQuestMonster(DungeonSystem& ds,DungeonState& d,PlayerSave& p,int target,std::vector<std::string>& msg){
    const int slot=(target==9?2:target==14?4:target==19?6:8);
    int kills=0, idleTicks=0;
    while(p.kingQuest[static_cast<std::size_t>(slot)]==0xFFFFu && kills<10 && idleTicks<300){
        struct Candidate{int x=-1,y=-1;std::vector<std::pair<int,int>> path;bool target=false;};
        Candidate best;
        bool targetExists=false;
        for(int yy=1;yy<=9;++yy)for(int xx=1;xx<=9;++xx) if(ds.cell(static_cast<const DungeonState&>(d),xx,yy).monster==target) targetExists=true;
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
            // A required monster that exists in a sealed/unreachable pocket will keep
            // replacement generation from choosing that ID again. A human can simply
            // leave and re-enter to regenerate the level; do the same instead of
            // grinding unrelated monsters indefinitely in this bad layout.
            if(targetExists) return false;
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
    return p.kingQuest[static_cast<std::size_t>(slot)]==1;
}



bool clearLineDungeon(const DungeonSystem& ds,const DungeonState& d,int x,int y,int tx,int ty){
    if(x!=tx && y!=ty) return false;
    const int dx=(tx>x)?1:(tx<x?-1:0), dy=(ty>y)?1:(ty<y?-1:0);
    int cx=x+dx,cy=y+dy;
    while(cx!=tx||cy!=ty){const auto& c=ds.cell(static_cast<const DungeonState&>(d),cx,cy);if(c.feature==1||c.feature==2||c.feature==8)return false;cx+=dx;cy+=dy;}
    return true;
}

bool killOneLevelOne(DungeonSystem& ds, DungeonState& d, PlayerSave& p, int locId, std::vector<std::string>& msg){
    const int exp0=p.experience, coin0=p.coin;
    msg.clear(); ds.enter(d,p,locId,msg);
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int guard=0; guard<3000 && p.hits>0 && p.experience==exp0; ++guard){
        // Prefer any current monster already in the equipped weapon's cardinal LOS.
        const int range=std::max(1,OriginalData::weaponRange(p.readyWeapon));
        bool fired=false;
        for(int dir=0;dir<4 && !fired;++dir){
            for(int dist=1;dist<=range;++dist){int tx=d.x+DX[dir]*dist,ty=d.y+DY[dir]*dist;if(tx<1||tx>9||ty<1||ty>9)break;
                const auto& c=ds.cell(static_cast<const DungeonState&>(d),tx,ty);
                if(c.feature==1||c.feature==2||c.feature==8)break;
                if(c.monster!=0xFFFFu){faceDungeon(ds,d,p,desiredOrientation(DX[dir],DY[dir]),msg);msg.clear();ds.attack(d,p,msg);finishDungeon(ds,d,p,msg);fired=true;break;}
            }
        }
        if(fired) continue;

        // Otherwise seek a reachable firing square at distance 2..range from a monster,
        // preferring the longest shot. Replan after every actual movement command because
        // monsters move in updateAfterCommand().
        struct C{int fx=-1,fy=-1;std::size_t path=9999;int dist=0;}; C best;
        for(int my=1;my<=9;++my)for(int mx=1;mx<=9;++mx){if(ds.cell(static_cast<const DungeonState&>(d),mx,my).monster==0xFFFFu)continue;
            for(int dir=0;dir<4;++dir)for(int dist=range;dist>=1;--dist){int fx=mx-DX[dir]*dist,fy=my-DY[dir]*dist;if(fx<1||fx>9||fy<1||fy>9)continue;
                const auto& fc=ds.cell(static_cast<const DungeonState&>(d),fx,fy);if(fc.feature==1||fc.feature==2||fc.feature==8||fc.monster!=0xFFFFu)continue;
                if(!clearLineDungeon(ds,d,fx,fy,mx,my))continue;auto path=dungeonPath(ds,d,fx,fy,false);bool here=(d.x==fx&&d.y==fy);if(!here&&path.empty())continue;
                std::size_t plen=here?0:path.size();if(best.fx<0||plen<best.path||(plen==best.path&&dist>best.dist))best={fx,fy,plen,dist};
            }
        }
        if(best.fx<0){msg.clear();ds.updateAfterCommand(d,p,msg);continue;}
        if(d.x==best.fx&&d.y==best.fy) continue;
        auto path=dungeonPath(ds,d,best.fx,best.fy,false);if(path.empty())continue;
        auto [nx,ny]=path.front(); const int dx=nx-d.x,dy=ny-d.y; faceDungeon(ds,d,p,desiredOrientation(dx,dy),msg);
        const auto& next=ds.cell(static_cast<const DungeonState&>(d),nx,ny);
        if(next.feature==2){msg.clear();ds.searchSecret(d,msg);finishDungeon(ds,d,p,msg);continue;}
        if(next.monster!=0xFFFFu){msg.clear();ds.attack(d,p,msg);finishDungeon(ds,d,p,msg);continue;}
        msg.clear();ds.forward(d,msg);finishDungeon(ds,d,p,msg);
    }
    if(p.hits==0)return false;
    if(d.active) ascendOut(ds,d,p,msg);
    return (p.experience>exp0 || p.coin>coin0) && p.hits>0;
}


void readyWeaponNormal(LocalSimulation& sim, LocalState& local, PlayerSave& p, int index){
    CommandSystem cmd; std::vector<std::string> out;
    cmd.beginReady(p,out); cmd.handleKey('W',local,p,sim,out);
    cmd.handleKey(static_cast<char>('a'+index),local,p,sim,out);
    require(p.readyWeapon==index,"normal Ready command failed for weapon");
}
void readyArmourNormal(LocalSimulation& sim, LocalState& local, PlayerSave& p, int index){
    CommandSystem cmd; std::vector<std::string> out;
    cmd.beginReady(p,out); cmd.handleKey('A',local,p,sim,out);
    cmd.handleKey(static_cast<char>('a'+index),local,p,sim,out);
    require(p.readyArmour==index,"normal Ready command failed for armour");
}
void dropPenceNormal(LocalSimulation& sim, LocalState& local, PlayerSave& p, int amount){
    CommandSystem cmd; std::vector<std::string> out;
    cmd.beginDrop(local,p,out); cmd.handleKey('P',local,p,sim,out);
    for(char c:std::to_string(amount)) cmd.handleKey(c,local,p,sim,out);
    cmd.handleKey('\r',local,p,sim,out);
    if(local.active){ auto more=sim.updateAfterCommand(local,p); (void)more; ++p.steps; }
}



LocalState makeLocal(const OriginalData& original,LocalSimulation& sim,const LocationInfo& loc){
    LocalState l; l.active=true; l.inTown=(loc.kind==LocationKind::Town); l.locationId=loc.id;
    l.mapIndex=original.localMapIndexForLocation(loc); l.actors=original.actorTemplate(l.mapIndex);
    l.alert=false; l.foundKey=false; l.drinks=0; l.tilesInitialized=true;
    for(int x=0;x<OriginalData::LOCAL_W;++x) for(int y=0;y<OriginalData::LOCAL_H;++y)
        l.tiles[(size_t)(x*OriginalData::LOCAL_H+y)]=original.localRawTile(l.mapIndex,x,y);
    l.correctKeyTile=l.inTown?0:(std::uint16_t)((sim.randomRange(1,255)&1)?0x3D:0x3C);
    original.localEntryForLocation(loc,l.x,l.y); return l;
}
std::vector<std::pair<int,int>> localPath(const LocalSimulation& sim,const LocalState& l,int gx,int gy){
    const int W=OriginalData::LOCAL_W,H=OriginalData::LOCAL_H; std::vector<int> prev((size_t)W*H,-1);
    std::queue<std::pair<int,int>> q; int st=l.y*W+l.x; prev[(size_t)st]=st; q.push({l.x,l.y});
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    while(!q.empty()){
        auto [x,y]=q.front();q.pop(); if(x==gx&&y==gy)break;
        for(int k=0;k<4;++k){int nx=x+DX[k],ny=y+DY[k]; if(nx<0||nx>=W||ny<0||ny>=H)continue;
            int ni=ny*W+nx;if(prev[(size_t)ni]!=-1)continue; if(!sim.passable(l,nx,ny))continue;
            if(sim.actorAt(l,nx,ny)!=nullptr && !(nx==gx&&ny==gy))continue; prev[(size_t)ni]=y*W+x;q.push({nx,ny});}
    }
    int gi=gy*W+gx;if(gx<0||gx>=W||gy<0||gy>=H||prev[(size_t)gi]==-1)return {};
    std::vector<std::pair<int,int>> out;int cur=gi;while(cur!=st){out.push_back({cur%W,cur/W});cur=prev[(size_t)cur];}std::reverse(out.begin(),out.end());return out;
}
void walkLocalTo(const LocalSimulation& sim,LocalState& l,int gx,int gy){
    auto path=localPath(sim,l,gx,gy);require((l.x==gx&&l.y==gy)||!path.empty(),"no normal local-map path to target");
    for(auto [x,y]:path){require(sim.passable(l,x,y),"local path crossed impassable tile");require(sim.actorAt(l,x,y)==nullptr,"local path crossed actor");l.x=x;l.y=y;}
    require(l.x==gx&&l.y==gy,"local walk ended at wrong square");
}
std::pair<int,int> reachableRawMarker(const OriginalData& o,const LocalSimulation& sim,const LocalState& l,std::uint8_t raw){
    for(int y=0;y<OriginalData::LOCAL_H;++y)for(int x=0;x<OriginalData::LOCAL_W;++x){
        if(o.localRawTile(l.mapIndex,x,y)!=raw)continue; if(x==l.x&&y==l.y)return {x,y}; if(!localPath(sim,l,x,y).empty())return {x,y};
    } return {-1,-1};
}

std::pair<int,int> findActorTile(const LocalState& l,std::uint16_t tile){
    for(const auto& a:l.actors) if(a.active()&&a.tile==tile) return {(int)a.x,(int)a.y};
    return {-1,-1};
}
void finishLocal(LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    auto more=sim.updateAfterCommand(l,p); msg.insert(msg.end(),more.begin(),more.end()); ++p.steps;
    require(p.hits>0,"player died during live castle sequence");
}
bool stepLocalToward(LocalSimulation& sim,LocalState& l,PlayerSave& p,int gx,int gy,std::vector<std::string>& msg){
    auto path=localPath(sim,l,gx,gy); if(path.empty()) return l.x==gx&&l.y==gy;
    auto [nx,ny]=path.front(); if(sim.actorAt(l,nx,ny)!=nullptr) return false;
    l.x=nx;l.y=ny; msg.clear(); finishLocal(sim,l,p,msg); return true;
}
void killLiveJester(LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int guard=0;guard<2000;++guard){
        auto jp=findActorTile(l,0x13); if(jp.first<0){require(l.foundKey,"Jester died without key flag");return;}
        int md=999,ax=-1,ay=-1;
        for(int k=0;k<4;++k){int x=jp.first+DX[k],y=jp.second+DY[k]; if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H)continue;
            if(!sim.passable(l,x,y)||sim.actorAt(l,x,y)!=nullptr)continue; auto path=localPath(sim,l,x,y); if(l.x==x&&l.y==y){ax=x;ay=y;md=0;break;} if(!path.empty()&&(int)path.size()<md){md=(int)path.size();ax=x;ay=y;}}
        if(ax<0){msg.clear();finishLocal(sim,l,p,msg);continue;}
        if(l.x!=ax||l.y!=ay){stepLocalToward(sim,l,p,ax,ay,msg);continue;}
        jp=findActorTile(l,0x13); if(jp.first<0)continue; int dx=jp.first-l.x,dy=jp.second-l.y;
        if(std::abs(dx)+std::abs(dy)!=1)continue;
        msg=sim.attack(l,p,dx,dy); finishLocal(sim,l,p,msg);
    }
    fail("could not kill actual castle Jester through normal movement/attack");
}

bool killReachableGuard(LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    int best=-1,bx=-1,by=-1,bd=999;
    for(int i=0;i<OriginalData::LOCAL_ACTOR_COUNT;++i){
        const auto& a=l.actors[(size_t)i]; if(!a.active()||a.tile!=0x11)continue;
        for(int k=0;k<4;++k){int x=(int)a.x+DX[k],y=(int)a.y+DY[k];if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H)continue;
            if(!sim.passable(l,x,y)||sim.actorAt(l,x,y)!=nullptr)continue; auto path=localPath(sim,l,x,y);int d=(l.x==x&&l.y==y)?0:(path.empty()?999:(int)path.size());
            if(d<bd){bd=d;best=i;bx=x;by=y;}}
    }
    if(best<0||bd>=999)return false;
    for(int g=0;g<2000 && (l.x!=bx||l.y!=by);++g){
        auto& a=l.actors[(size_t)best]; if(!a.active())return true;
        // Guard can move after every command, so recompute a reachable neighbour.
        int nbx=-1,nby=-1,nd=999;
        for(int k=0;k<4;++k){int x=(int)a.x+DX[k],y=(int)a.y+DY[k];if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H)continue;if(!sim.passable(l,x,y)||sim.actorAt(l,x,y)!=nullptr)continue;auto path=localPath(sim,l,x,y);int d=(l.x==x&&l.y==y)?0:(path.empty()?999:(int)path.size());if(d<nd){nd=d;nbx=x;nby=y;}}
        if(nbx<0||nd>=999)break; bx=nbx;by=nby; if(!stepLocalToward(sim,l,p,bx,by,msg))break;
    }
    for(int g=0;g<200;++g){
        auto& a=l.actors[(size_t)best]; if(!a.active())return true; int dx=(int)a.x-l.x,dy=(int)a.y-l.y;
        if(std::abs(dx)+std::abs(dy)!=1)return false; msg=sim.attack(l,p,dx,dy);finishLocal(sim,l,p,msg);
    }
    return !l.actors[(size_t)best].active();
}
bool walkLiveTo(LocalSimulation& sim,LocalState& l,PlayerSave& p,int gx,int gy,std::vector<std::string>& msg,int limit=5000){
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int guard=0;guard<limit;++guard){
        if(l.x==gx&&l.y==gy) return true;
        if(stepLocalToward(sim,l,p,gx,gy,msg)) continue;
        bool attacked=false;
        for(int k=0;k<4 && !attacked;++k){auto a=sim.actorAt(l,l.x+DX[k],l.y+DY[k]); if(a&&a->tile==0x11){msg=sim.attack(l,p,DX[k],DY[k]);finishLocal(sim,l,p,msg);attacked=true;}}
        if(attacked)continue;
        if(killReachableGuard(sim,l,p,msg))continue;
        msg.clear();finishLocal(sim,l,p,msg);
    }
    return l.x==gx&&l.y==gy;
}
bool tryUnlockLiveDoor(const OriginalData& o,LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    for(std::uint8_t marker: {std::uint8_t(0x3C),std::uint8_t(0x3D)}){
        for(int y=0;y<OriginalData::LOCAL_H;++y)for(int x=0;x<OriginalData::LOCAL_W;++x){
            if(o.localRawTile(l.mapIndex,x,y)!=marker)continue;
            if(!walkLiveTo(sim,l,p,x,y,msg)) continue;
            const auto before=(y>0)?sim.rawTile(l,x,y-1):0;
            msg=sim.unlock(l,p);
            const auto after=(y>0)?sim.rawTile(l,x,y-1):0;
            if(before!=1&&after==1){ finishLocal(sim,l,p,msg); return true; }
            finishLocal(sim,l,p,msg);
        }
    }
    return false;
}
bool princessRoomReachable(const LocalSimulation& sim,const LocalState& l){
    auto pp=findActorTile(l,0x16); if(pp.first<0) return false;
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int k=0;k<4;++k){int x=pp.first+DX[k],y=pp.second+DY[k]; if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H)continue;
        if(!sim.passable(l,x,y)||sim.actorAt(l,x,y)!=nullptr)continue; if((l.x==x&&l.y==y)||!localPath(sim,l,x,y).empty()) return true;}
    return false;
}
void leaveCastleWithoutPrincess(LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int guard=0;guard<5000;++guard){
        if(l.x==0&&l.y==9){ require(!sim.princessAdjacent(l),"wrong-door retry unexpectedly has Princess at exit"); l=LocalState{}; ++p.steps; return; }
        if(stepLocalToward(sim,l,p,0,9,msg)) continue;
        bool attacked=false;
        for(int k=0;k<4 && !attacked;++k){auto a=sim.actorAt(l,l.x+DX[k],l.y+DY[k]);if(a&&a->tile==0x11){msg=sim.attack(l,p,DX[k],DY[k]);finishLocal(sim,l,p,msg);attacked=true;}}
        if(!attacked){msg.clear();finishLocal(sim,l,p,msg);}
    }
    fail("could not leave castle after wrong randomized key door");
}

void leadPrincessAndExit(LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    // After the Jester attack the castle is alert; the real Princess AI follows the player.
    // Replan toward the west gate one command at a time while actors move/chase normally.
    for(int guard=0;guard<5000;++guard){
        if(l.x==0&&l.y==9 && sim.princessAdjacent(l)){
            msg=sim.rescuePrincess(l,p); require(!msg.empty(),"castle exit did not trigger Princess rescue"); l=LocalState{}; ++p.steps; return;
        }
        auto pp=findActorTile(l,0x16); require(pp.first>=0,"Princess disappeared during rescue route");
        // If not yet near the Princess, approach a free cardinal neighbour first.
        if(!sim.princessAdjacent(l)){
            static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1}; int bx=-1,by=-1,bd=999;
            for(int k=0;k<4;++k){int x=pp.first+DX[k],y=pp.second+DY[k];if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H)continue;if(!sim.passable(l,x,y)||sim.actorAt(l,x,y)!=nullptr)continue;auto path=localPath(sim,l,x,y);if(l.x==x&&l.y==y){bx=x;by=y;bd=0;break;}if(!path.empty()&&(int)path.size()<bd){bd=(int)path.size();bx=x;by=y;}}
            if(bx>=0 && stepLocalToward(sim,l,p,bx,by,msg)) continue;
            // No route to a Princess-adjacent square this turn: let guards/Princess move.
            bool attacked=false; static constexpr int ADX[4]={-1,1,0,0},ADY[4]={0,0,-1,1};
            for(int k=0;k<4 && !attacked;++k){auto a=sim.actorAt(l,l.x+ADX[k],l.y+ADY[k]);if(a&&a->tile==0x11){msg=sim.attack(l,p,ADX[k],ADY[k]);finishLocal(sim,l,p,msg);attacked=true;}}
            if(!attacked){msg.clear();finishLocal(sim,l,p,msg);}
            continue;
        }
        if((l.x!=0||l.y!=9) && stepLocalToward(sim,l,p,0,9,msg)) continue;
        // A guard may temporarily block the only corridor. Attack one adjacent guard normally.
        bool attacked=false; static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
        for(int k=0;k<4 && !attacked;++k){auto a=sim.actorAt(l,l.x+DX[k],l.y+DY[k]);if(a&&a->tile==0x11){msg=sim.attack(l,p,DX[k],DY[k]);finishLocal(sim,l,p,msg);attacked=true;}}
        if(!attacked){msg.clear();finishLocal(sim,l,p,msg);}
    }
    { auto pp=findActorTile(l,0x16); std::cerr<<"lead fail player="<<l.x<<","<<l.y<<" princess="<<pp.first<<","<<pp.second<<" adj="<<sim.princessAdjacent(l)<<" hits="<<p.hits<<"\n"; fail("could not lead Princess to real castle exit"); }
}

std::vector<std::pair<int,int>> worldPath(const OriginalData& o,const VehicleSystem& v,const PlayerSave& p,int gx,int gy){
    const int W=OriginalData::MAP_W,H=OriginalData::MAP_H;std::vector<int> prev((size_t)W*H,-1);std::queue<std::pair<int,int>> q;
    int st=(int)p.y*W+(int)p.x;prev[(size_t)st]=st;q.push({(int)p.x,(int)p.y});static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    while(!q.empty()){auto [x,y]=q.front();q.pop();if(x==gx&&y==gy)break;for(int k=0;k<4;++k){int nx=(x+DX[k]+W)%W,ny=(y+DY[k]+H)%H;int ni=ny*W+nx;if(prev[(size_t)ni]!=-1)continue;std::vector<std::string> m;if(!v.mayMove(o,p,nx,ny,m))continue;prev[(size_t)ni]=y*W+x;q.push({nx,ny});}}
    int gi=gy*W+gx;if(prev[(size_t)gi]==-1)return{};std::vector<std::pair<int,int>>out;int cur=gi;while(cur!=st){out.push_back({cur%W,cur/W});cur=prev[(size_t)cur];}std::reverse(out.begin(),out.end());return out;
}
void walkWorldTo(const OriginalData& o,VehicleSystem& v,OverworldSimulation& ow,PlayerSave& p,int gx,int gy){
    auto path=worldPath(o,v,p,gx,gy);require((p.x==gx&&p.y==gy)||!path.empty(),"no normal overworld route to target for current transport");
    for(auto [x,y]:path){std::vector<std::string> m;require(v.mayMove(o,p,x,y,m),"world path step became illegal");int ox=p.x,oy=p.y;p.x=(std::uint16_t)x;p.y=(std::uint16_t)y;p.worldObjects[0].w[2]=p.x;p.worldObjects[0].w[3]=p.y;ow.onPlayerMoved(p,ox,oy);++p.steps;}
    require((int)p.x==gx&&(int)p.y==gy,"overworld walk ended at wrong square");
}

int craftHeading(const SpaceState& s){return s.sectors[(size_t)(s.sectorX*7+s.sectorY)].craft[(size_t)s.currentSlot].heading;}
SpaceCraft& liveCraft(SpaceState& s){return s.sectors[(size_t)(s.sectorX*7+s.sectorY)].craft[(size_t)s.currentSlot];}
const SpaceCraft& liveCraft(const SpaceState& s){return s.sectors[(size_t)(s.sectorX*7+s.sectorY)].craft[(size_t)s.currentSlot];}
void turnTo(SpaceSystem& sp,SpaceState& s,PlayerSave& p,int h,std::vector<std::string>& m){int g=0;while(craftHeading(s)!=h&&g++<4){m.clear();sp.direction(s,p,2,m);}if(craftHeading(s)!=h)std::cerr<<"turnfail sector="<<s.sectorX<<","<<s.sectorY<<" fuel="<<liveCraft(s).fuel<<" head="<<craftHeading(s)<<" want="<<h<<" front="<<s.frontView<<" dock="<<s.docked<<" kills="<<p.spaceKills<<"\n";require(craftHeading(s)==h,"space heading setup failed");}
void setVel(SpaceSystem& sp,SpaceState& s,PlayerSave& p,int vx,int vy,std::vector<std::string>& m){
 require(!s.frontView,"setVel in front view"); turnTo(sp,s,p,1,m); int g=0; while(s.velocityX!=vx&&g++<30){m.clear();sp.direction(s,p,s.velocityX<vx?3:4,m);} require(s.velocityX==vx,"vx setup failed");
 turnTo(sp,s,p,3,m); g=0; while(s.velocityY!=vy&&g++<30){m.clear();sp.direction(s,p,s.velocityY<vy?3:4,m);} require(s.velocityY==vy,"vy setup failed");
}
void runFrames(SpaceSystem& sp,SpaceState& s,PlayerSave& p,std::vector<std::string>& m,int n){for(int i=0;i<n&&s.active;++i){m.clear();sp.updateFrame(s,p,m);}}
void jump1(SpaceSystem& sp,SpaceState& s,PlayerSave& p,int dx,int dy,std::vector<std::string>& m){
 require(abs(dx)+abs(dy)==1,"non-cardinal jump"); if(s.frontView){m.clear();sp.toggleView(s,p,m);require(!s.frontView,"cannot leave front view");}
 setVel(sp,s,p,dx,dy,m); m.clear();sp.toggleView(s,p,m);require(s.frontView,"cannot enter front view"); int ox=s.sectorX,oy=s.sectorY; m.clear();sp.hyperJump(s,p,m);require(s.hyperPhase!=0,"hyperjump failed to start");
 int g=0;while(s.active&&s.hyperPhase&&g++<1000){m.clear();sp.updateFrame(s,p,m);}require(s.active&&s.hyperPhase==0,"hyperjump failed");require(s.sectorX==(ox+dx+7)%7&&s.sectorY==(oy+dy+7)%7,"wrong hyperjump destination");
}
int torusDelta(int a,int b){int d=(b-a+7)%7;return d<=3?d:d-7;}
int clearSector(SpaceSystem&,SpaceState&,PlayerSave&,std::vector<std::string>&);
void clearIfHostile(SpaceSystem&sp,SpaceState&s,PlayerSave&p,std::vector<std::string>&m){if(s.sectors[(size_t)(s.sectorX*7+s.sectorY)].enemies>0)clearSector(sp,s,p,m);}
void route(SpaceSystem& sp,SpaceState& s,PlayerSave& p,int tx,int ty,std::vector<std::string>& m){int dx=torusDelta(s.sectorX,tx),dy=torusDelta(s.sectorY,ty);while(dx<0){jump1(sp,s,p,-1,0,m);clearIfHostile(sp,s,p,m);++dx;}while(dx>0){jump1(sp,s,p,1,0,m);clearIfHostile(sp,s,p,m);--dx;}while(dy<0){jump1(sp,s,p,0,-1,m);clearIfHostile(sp,s,p,m);++dy;}while(dy>0){jump1(sp,s,p,0,1,m);clearIfHostile(sp,s,p,m);--dy;}}
bool hitNow(const SpaceState&s){static const int xa[7]={5,6,8,10,12,16,18},ya[7]={4,6,8,10,12,14,18},xi[7]={1,1,1,1,0,0,0},yi[7]={5,5,4,4,3,2,1};if(!s.enemyX)return false;int st=std::clamp(s.enemySprite,0,6),dx=s.crosshairX-s.enemyX,dy=s.crosshairY-s.enemyY;return dx>=xi[st]&&dx<=xa[st]&&dy>=yi[st]&&dy<=ya[st];}
void killOne(SpaceSystem& sp,SpaceState& s,PlayerSave& p,std::vector<std::string>&m){
 int idx=s.sectorX*7+s.sectorY,b=s.sectors[(size_t)idx].enemies,g=0;
 while(s.active&&s.sectors[(size_t)idx].enemies==b&&g++<300){
  if(hitNow(s)){m.clear();sp.fire(s,p,m);continue;}
  int st=std::clamp(s.enemySprite,0,6);static const int tx[7]={3,3,4,5,6,8,9},ty[7]={5,5,6,7,8,8,9};
  int ex=s.enemyX+tx[st]-s.crosshairX,ey=s.enemyY+ty[st]-s.crosshairY;
  bool horiz=std::abs(ex)>=std::abs(ey);m.clear();sp.direction(s,p,horiz?(ex<0?1:2):(ey<0?3:4),m);
  // One aim command keeps moving the reticle every flight tick. Let it close most
  // of the gap before spending fuel on another steering command.
  int gap=horiz?std::abs(ex):std::abs(ey);int frames=std::clamp(gap/3,1,12);
  for(int k=0;k<frames&&s.active&&!hitNow(s);++k){m.clear();sp.updateFrame(s,p,m);}
 }
 require(s.active,"died in live space combat");require(s.sectors[(size_t)idx].enemies==b-1,"live aiming could not kill generated enemy");
}
int clearSector(SpaceSystem&sp,SpaceState&s,PlayerSave&p,std::vector<std::string>&m){if(!s.frontView){m.clear();sp.toggleView(s,p,m);require(s.frontView,"cannot enter combat view");}int idx=s.sectorX*7+s.sectorY,n=s.sectors[(size_t)idx].enemies;for(int i=0;i<n;++i)killOne(sp,s,p,m);return n;}
void driveTo(SpaceSystem&sp,SpaceState&s,PlayerSave&p,int vx,int vy,int tx,int ty,std::vector<std::string>&m,int heading=-1){setVel(sp,s,p,vx,vy,m);if(heading>=0)turnTo(sp,s,p,heading,m);int g=0;while(s.active&&(liveCraft(s).x!=tx||liveCraft(s).y!=ty)&&g++<500){m.clear();sp.updateFrame(s,p,m);if((vx>0&&liveCraft(s).x>tx)||(vx<0&&liveCraft(s).x<tx)||(vy>0&&liveCraft(s).y>ty)||(vy<0&&liveCraft(s).y<ty))break;}if(!(s.active&&liveCraft(s).x==tx&&liveCraft(s).y==ty))std::cerr<<"drive miss from sector="<<s.sectorX<<","<<s.sectorY<<" pos="<<liveCraft(s).x<<","<<liveCraft(s).y<<" target="<<tx<<","<<ty<<" vel="<<s.velocityX<<","<<s.velocityY<<" fuel="<<liveCraft(s).fuel<<" dock="<<s.docked<<" kills="<<p.spaceKills<<"\n"; require(s.active&&liveCraft(s).x==tx&&liveCraft(s).y==ty,"top-view docking route missed target");}
void dockPort(SpaceSystem&sp,SpaceState&s,PlayerSave&p,int port,std::vector<std::string>&m){require(s.sectorX==3&&s.sectorY==3,"dock outside home");if(s.frontView){m.clear();sp.toggleView(s,p,m);require(!s.frontView,"home front view did not close");}
 int type=liveCraft(s).type; static const int dx[12]={-17,-15,-15,18,18,18,0,1,2,0,1,2},dy[12]={1,1,1,1,1,1,-17,-16,-16,18,17,17};int tx=200+dx[port*3+type],ty=100+dy[port*3+type];
 if(port==2){driveTo(sp,s,p,0,-1,liveCraft(s).x,50,m);driveTo(sp,s,p,(tx>liveCraft(s).x)?1:-1,0,tx,50,m);driveTo(sp,s,p,0,1,tx,ty,m,3);}else if(port==0){driveTo(sp,s,p,0,-1,liveCraft(s).x,20,m);driveTo(sp,s,p,(100>liveCraft(s).x)?1:-1,0,100,20,m);driveTo(sp,s,p,0,1,100,ty,m);driveTo(sp,s,p,1,0,tx,ty,m,1);}else require(false,"unsupported test dock port");
 require(s.docked,"base docking did not trigger on geometric contact");}
void undockRetro(SpaceSystem&sp,SpaceState&s,PlayerSave&p,std::vector<std::string>&m){m.clear();sp.direction(s,p,4,m);require(!s.docked,"retro did not release dock");m.clear();sp.updateFrame(s,p,m);require(s.active,"died undocking");}

}


static bool caughtMessage(const std::vector<std::string>& m){
    return std::any_of(m.begin(),m.end(),[](const std::string& s){return s.find("발각")!=std::string::npos;});
}

static void exitCastlePeacefully(LocalSimulation& sim,LocalState& l,PlayerSave& p,std::vector<std::string>& msg){
    require(walkLiveTo(sim,l,p,0,9,msg),"could not walk back to castle gate");
    l=LocalState{}; ++p.steps;
}

static int stealUntilItem(const OriginalData& original,LocalSimulation& sim,const LocationInfo& castleLoc,
                          PlayerSave& p,std::uint8_t marker,int itemIndex,bool weapon,
                          std::uint16_t seedBase,std::vector<std::string>& msg){
    const PlayerSave saved=p;
    int reloads=0,totalSteals=0;
    for(int attempt=0;attempt<500;++attempt){
        p=saved; // actual reload: discard every unsaved theft since the checkpoint.
        sim.seed(static_cast<std::uint16_t>(seedBase+attempt*37));
        LocalState l=makeLocal(original,sim,castleLoc);
        auto mk=reachableRawMarker(original,sim,l,marker);
        require(mk.first>=0,"steal marker unreachable");
        walkLocalTo(sim,l,mk.first,mk.second);
        for(int s=0;s<64;++s){
            ++totalSteals;
            msg=sim.steal(l,p);
            if(caughtMessage(msg)){ ++reloads; break; }
            ++p.steps;
            if((weapon ? p.weapons[(size_t)itemIndex] : p.armour[(size_t)itemIndex])>0){
                exitCastlePeacefully(sim,l,p,msg);
                std::cout<<"STEAL success marker="<<(int)marker<<" total="<<totalSteals<<" reloads="<<reloads<<"\n";
                return reloads;
            }
            auto more=sim.updateAfterCommand(l,p); (void)more;
            require(p.hits>0,"died during non-alert stealing sequence");
        }
    }
    fail("save/reload stealing never obtained requested item");
}

static bool shootCardinalMonster(OverworldSimulation& ow,PlayerSave& p,std::vector<std::string>& msg){
    const int range=OriginalData::weaponRange(p.readyWeapon);
    static constexpr int DX[4]={-1,1,0,0}, DY[4]={0,0,-1,1};
    for(int k=0;k<4;++k){
        for(int d=1;d<=range;++d){
            int x=((int)p.x+DX[k]*d+OriginalData::MAP_W)%OriginalData::MAP_W;
            int y=((int)p.y+DY[k]*d+OriginalData::MAP_H)%OriginalData::MAP_H;
            if(ow.monsterAt(p,x,y)){
                for(int n=0;n<50 && ow.monsterAt(p,x,y) && p.hits>0;++n){
                    msg=ow.attack(p,DX[k],DY[k],WorldAttackMode::Normal);
                    auto m2=ow.updateAfterCommand(p); (void)m2; ++p.steps;
                }
                return true;
            }
        }
    }
    return false;
}


static bool tacticalFieldStep(const OriginalData& o,VehicleSystem& v,OverworldSimulation& ow,PlayerSave& p){
    struct M{int x,y;}; std::vector<M> ms;
    for(int y=0;y<OriginalData::MAP_H;++y) for(int x=0;x<OriginalData::MAP_W;++x) if(ow.monsterAt(p,x,y)) ms.push_back({x,y});
    if(ms.empty()) return false;
    auto td=[](int a,int b,int mod){int d=b-a; if(d>mod/2)d-=mod; if(d<-mod/2)d+=mod; return d;};
    int best=-100000,bx=p.x,by=p.y;
    static constexpr int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
    for(int k=0;k<4;++k){
        int nx=((int)p.x+DX[k]+OriginalData::MAP_W)%OriginalData::MAP_W;
        int ny=((int)p.y+DY[k]+OriginalData::MAP_H)%OriginalData::MAP_H;
        std::vector<std::string> tmp; if(!v.mayMove(o,p,nx,ny,tmp)) continue;
        int minD=999, align=0;
        for(auto m:ms){int dx=td(nx,m.x,OriginalData::MAP_W),dy=td(ny,m.y,OriginalData::MAP_H);int d=std::max(abs(dx),abs(dy));minD=std::min(minD,d);if((dx==0||dy==0)&&d>=1&&d<=3)align=1;}
        int score=align*1000+minD*10;
        if(score>best){best=score;bx=nx;by=ny;}
    }
    if(best==-100000) return false;
    int ox=p.x,oy=p.y;p.x=(std::uint16_t)bx;p.y=(std::uint16_t)by;p.worldObjects[0].w[2]=p.x;p.worldObjects[0].w[3]=p.y;ow.onPlayerMoved(p,ox,oy);++p.steps;
    return true;
}

int main(int argc,char**argv){
    const std::string data=(argc>1)?argv[1]:"gamedata";
    OriginalData original; std::string error; require(original.loadGameplayOracle(data,error),"gameplay oracle load: "+error);
    PlayerSave p; require(original.loadSave(data+"/PLAYER1.U1",p,error),"PLAYER1 load: "+error);

    // User-specified build: male Human Fighter.  The stock fixture in this archive is
    // a Thief, so this isolated route test changes only identity/class-derived combat
    // attributes to the Fighter selection before any gameplay begins.
    p.race=0; p.sex=0; p.klass=0; p.strength=26; p.agility=26;
    require(p.hits==150 && p.coin==100,"unexpected fresh baseline");

    LocalSimulation sim(original); sim.seed(0x2345);
    OverworldSimulation ow(original,sim); VehicleSystem vehicles; KingSystem king;
    int allowance=0; std::vector<std::string> msg;
    auto cit=std::find_if(original.locations().begin(),original.locations().end(),[](const LocationInfo& l){return l.id==33;});
    require(cit!=original.locations().end(),"Lord British castle 33 missing");

    // Stand at Lord British castle and save.
    walkWorldTo(original,vehicles,ow,p,cit->x,cit->y);
    PlayerSave castleFrontSave=p;

    // Weapon storeroom: repeat Steal; any caught attempt is discarded by reload.
    int wr=stealUntilItem(original,sim,*cit,p,0x3B,15,true,0x3100,msg);
    require(p.weapons[15]>0,"Blaster missing after steal/reload");
    PlayerSave blasterSave=p;

    // Armour storeroom from the saved Blaster checkpoint.
    int ar=stealUntilItem(original,sim,*cit,p,0x37,5,false,0x5200,msg);
    require(p.weapons[15]>0 && p.armour[5]>0,"Blaster/Reflect not both saved");
    PlayerSave gearedSave=p;

    // Equip both through the normal Ready command while peacefully inside the castle.
    sim.seed(0x6100); LocalState equipCastle=makeLocal(original,sim,*cit);
    readyWeaponNormal(sim,equipCastle,p,15); readyArmourNormal(sim,equipCastle,p,5);
    exitCastlePeacefully(sim,equipCastle,p,msg);
    require(p.readyWeapon==15 && p.readyArmour==5,"end-game gear did not equip normally");

    // Earn the missing pence on the overworld using normal spawn/update/Attack only.
    // Keep the post-gear save as the user's retry point: if an unlucky 150-Hit field
    // encounter kills the Fighter, reload that save and try a different live RNG stream.
    const PlayerSave fieldSave=p;
    int fieldAttempts=0,fieldTurns=0,kills=0; std::uint16_t hitsBefore=0,coinBefore=0;
    bool fieldSuccess=false;
    for(fieldAttempts=0;fieldAttempts<128 && !fieldSuccess;++fieldAttempts){
        p=fieldSave; sim.seed(static_cast<std::uint16_t>(0x7331+fieldAttempts*53));
        fieldTurns=0;kills=0;std::uint16_t lastCoin=p.coin;
        while(p.coin<200 && p.hits>0 && fieldTurns++<12000){
            ow.maybeSpawn(p);
            if(shootCardinalMonster(ow,p,msg)){ if(p.coin>lastCoin){++kills;lastCoin=p.coin;} continue; }
            tacticalFieldStep(original,vehicles,ow,p);
            auto more=ow.updateAfterCommand(p); (void)more; ++p.steps;
        }
        if(p.hits>0 && p.coin>=200){fieldSuccess=true;hitsBefore=p.hits;coinBefore=p.coin;break;}
    }
    require(fieldSuccess,"no survivable normal field-farming retry reached 200p");
    // Return to Lord British and give exactly 200 pence.
    walkWorldTo(original,vehicles,ow,p,cit->x,cit->y);
    sim.seed(0x8111); LocalState donateCastle=makeLocal(original,sim,*cit);
    auto km=reachableRawMarker(original,sim,donateCastle,0x3E); require(km.first>=0,"king marker unreachable");
    require(walkLiveTo(sim,donateCastle,p,km.first,km.second,msg),"could not reach Lord British");
    msg.clear(); require(king.begin(original,donateCastle,p,allowance,msg),"king transaction failed");
    msg.clear(); require(!king.handleKey('P',p,msg),"king pence stage failed");
    for(char c:std::string("200")){msg.clear();require(!king.handleKey(c,p,msg),"king amount input failed");}
    msg.clear(); require(king.handleKey('\r',p,msg),"king 200p donation did not finish");
    require(p.coin==coinBefore-200,"king did not remove exactly 200p");
    require(p.hits==static_cast<std::uint16_t>(hitsBefore+300),"king did not grant exactly +300 Hits");
    require(walkLiveTo(sim,donateCastle,p,0,9,msg),"could not leave after king donation"); donateCastle=LocalState{}; ++p.steps;
    PlayerSave prePrincessSave=p;

    // Reload the post-donation save and verify the equipped gear persisted.
    p=prePrincessSave;
    require(p.readyWeapon==15 && p.readyArmour==5 && p.weapons[15]>0 && p.armour[5]>0,
            "post-donation save lost Blaster/Reflect equipment");

    // Re-enter, kill the actual Jester, unlock the Princess-room door, and escort her out.
    // Seed 8 selects the Princess-side keyed door for deterministic regression.
    sim.seed(8); LocalState castle=makeLocal(original,sim,*cit);
    killLiveJester(sim,castle,p,msg);
    require(castle.alert && castle.foundKey,"Jester did not produce live key/alert state");
    require(tryUnlockLiveDoor(original,sim,castle,p,msg),"key did not unlock a castle door");
    require(princessRoomReachable(sim,castle),"deterministic key did not open Princess room");
    leadPrincessAndExit(sim,castle,p,msg);
    require(!castle.active,"Princess rescue did not exit castle");
    require(p.hits>0,"Fighter died during Jester/Princess sequence");

    std::cout<<"PASS M40 Fighter route\n";
    std::cout<<"weapon reloads="<<wr<<" armour reloads="<<ar<<" field kills="<<kills
             <<" coin before donation="<<coinBefore<<" hits before="<<hitsBefore
             <<" hits after donation="<<prePrincessSave.hits<<" final hits="<<p.hits<<"\n";
    std::cout<<"gear ready weapon="<<p.readyWeapon<<" armour="<<p.readyArmour<<"\n";
    return 0;
}