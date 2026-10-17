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

namespace {
[[noreturn]] void fail(const std::string& s){ std::cerr << "M38 campaign smoke FAIL: " << s << "\n"; std::exit(1); }
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
    while(p.kingQuest[static_cast<std::size_t>(slot)]==0xFFFFu && kills<40 && idleTicks<2000){
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
    return p.kingQuest[static_cast<std::size_t>(slot)]==1;
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

int main(int argc,char**argv){
    if(argc!=2){ std::cerr<<"usage: campaign_smoke_m38 /path/to/gamedata\n"; return 2; }
    const std::string data=argv[1];
    OriginalData original; std::string error;
    require(original.loadGameplayOracle(data,error),"gameplay oracle load: "+error);

    PlayerSave p;
    require(original.loadSave(data+"/PLAYER1.U1",p,error),"fresh PLAYER1 load: "+error);
    p.name="M38TEST";
    p.hits=9999; p.food=9999; p.coin=9999;
    p.strength=99; p.agility=99; p.stamina=99; p.wisdom=99; p.intelligence=99;
    p.readyWeapon=15; p.weapons[15]=1; p.readyArmour=5; p.armour[5]=1;
    p.experience=5000; p.spaceKills=0;
    p.kingQuest.fill(0); p.gems.fill(0);

    LocalSimulation sim(original); sim.seed(0x1234);
    OverworldSimulation overworld(original,sim);
    DungeonSystem dungeonSystem(original,sim);
    KingSystem king;
    int storeroomAllowance=0;
    std::vector<std::string> msg;

    // M36: acquire campaign transport through an actual town entry and local walk.
    // Start from the coordinates present in the supplied PLAYER1 save, walk on foot to Britian,
    // enter at the real town gate, walk to the Transport counter, and buy Aircar + Shuttle.
    p.transport=0; p.worldObjects[0].w[0]=10; p.worldObjects[0].w[2]=p.x; p.worldObjects[0].w[3]=p.y;
    p.steps=4001; // Aircar availability threshold; progression stat only, not a coordinate shortcut.
    VehicleSystem vehicles;
    const auto townIt=std::find_if(original.locations().begin(),original.locations().end(),[](const LocationInfo& l){return l.id==1;});
    require(townIt!=original.locations().end(),"Britian missing from location oracle");
    walkWorldTo(original,vehicles,overworld,p,townIt->x,townIt->y);
    LocalState town=makeLocal(original,sim,*townIt); auto transportMarker=reachableRawMarker(original,sim,town,0x3E);
    require(transportMarker.first>=0,"no reachable Transport counter from actual town entry"); walkLocalTo(sim,town,transportMarker.first,transportMarker.second);
    ShopSystem shop; msg.clear(); require(shop.begin(original,town,p,msg),"actual town Transport shop did not open");
    msg.clear(); require(!shop.handleKey('B',original,town,p,sim,msg),"transport shop buy stage ended unexpectedly");
    msg.clear(); require(shop.handleKey('E',original,town,p,sim,msg),"Aircar purchase did not finish");
    msg.clear(); require(shop.begin(original,town,p,msg),"Transport shop did not reopen for Shuttle");
    msg.clear(); require(!shop.handleKey('B',original,town,p,sim,msg),"Shuttle buy stage ended unexpectedly");
    msg.clear(); require(shop.handleKey('F',original,town,p,sim,msg),"Shuttle purchase did not finish");
    walkLocalTo(sim,town,19,17); town=LocalState{}; // normal town gate return to OUT map.
    const int aircarObject=findObjectTile(p,0x10); require(aircarObject>0,"Aircar purchase did not spawn world object");
    walkWorldTo(original,vehicles,overworld,p,p.worldObjects[(size_t)aircarObject].x(),p.worldObjects[(size_t)aircarObject].y());
    msg.clear();vehicles.board(p,false,msg);require(p.transport==5,"normal Board did not enter purchased Aircar");
    const int shuttleObject=findObjectTile(p,0x11);require(shuttleObject>0,"Shuttle purchase did not spawn world object");

    // Assign all four mandatory king quests by actually travelling to each castle,
    // entering through its real gate, and walking to a reachable king-service marker.
    static constexpr int CASTLES[4]={2,4,6,8};
    static constexpr int MONSTERS[4]={9,14,19,24};
    static constexpr int LEVELS[4]={3,5,7,9};
    for(int z=0;z<4;++z){
        const int castle=CASTLES[z];
        const auto it=std::find_if(original.locations().begin(),original.locations().end(),[castle](const LocationInfo& l){return l.id==32+castle;});
        require(it!=original.locations().end(),"castle location missing from OUT oracle");
        walkWorldTo(original,vehicles,overworld,p,it->x,it->y);
        LocalState local=makeLocal(original,sim,*it);auto km=reachableRawMarker(original,sim,local,0x3E);require(km.first>=0,"king service marker unreachable from castle entry");walkLocalTo(sim,local,km.first,km.second);
        msg.clear(); require(king.begin(original,local,p,storeroomAllowance,msg),"king service dialogue did not open");
        msg.clear(); require(king.handleKey('S',p,msg),"king service selection did not complete");
        require(p.kingQuest[(size_t)castle]==0xFFFFu,"king did not assign expected dungeon quest");
        walkLocalTo(sim,local,0,9); local=LocalState{};
    }

    // Enter one real original dungeon and descend through the generated levels using
    // normal turn/search/forward/climb/attack commands.  Quest monsters are never
    // inserted by the test; if absent, ordinary kills exercise OUT.EXE's replacement
    // generator until the required member of that five-monster level band appears.
    const auto dit=std::find_if(original.locations().begin(),original.locations().end(),
                                [](const LocationInfo& l){return l.kind==LocationKind::Dungeon;});
    require(dit!=original.locations().end(),"no original dungeon location found");
    walkWorldTo(original,vehicles,overworld,p,dit->x,dit->y);
    DungeonState d;
    for(int z=0;z<4;++z){
        bool done=false;
        for(int attempt=0;attempt<40 && !done;++attempt){
            msg.clear(); dungeonSystem.enter(d,p,dit->id,msg);
            descendToLevel(dungeonSystem,d,p,LEVELS[z],msg);
            done=huntQuestMonster(dungeonSystem,d,p,MONSTERS[z],msg);
            ascendOut(dungeonSystem,d,p,msg);
            require(!d.active,"quest dungeon did not exit through the real up-ladder chain");
        }
        require(done && p.kingQuest[static_cast<std::size_t>(CASTLES[z])]==1,"generated dungeon retries did not complete mandatory quest");
    }

    // Return to each actual quest castle for the normal immediate gem reward.
    for(int z=0;z<4;++z){
        const int castle=CASTLES[z];
        const auto it=std::find_if(original.locations().begin(),original.locations().end(),
                                   [castle](const LocationInfo& l){return l.id==32+castle;});
        require(it!=original.locations().end(),"reward castle missing");
        walkWorldTo(original,vehicles,overworld,p,it->x,it->y);
        LocalState local=makeLocal(original,sim,*it); auto km=reachableRawMarker(original,sim,local,0x3E);
        require(km.first>=0,"reward king marker unreachable"); walkLocalTo(sim,local,km.first,km.second);
        msg.clear(); const bool dialogue=king.begin(original,local,p,storeroomAllowance,msg);
        require(!dialogue,"completed quest should reward immediately rather than reopen dialogue");
        require(p.gems[static_cast<std::size_t>(z)]>0,"expected quadrant gem not awarded");
    }
    require(std::all_of(p.gems.begin(),p.gems.end(),[](std::uint16_t n){return n>0;}),"four-gem gate unreachable");
    std::cout<<"PASS actual overworld/castle traversal + generated dungeon combat + real ascent -> four gems\n";

    // With the real per-command monster pass enabled, a Gelatinous Cube can
    // destroy the equipped suit. Revisit a real armour counter and buy Reflect
    // Armour before SPACE.EXE's vacuum docking check.
    walkWorldTo(original,vehicles,overworld,p,townIt->x,townIt->y);
    LocalState armourTown=makeLocal(original,sim,*townIt); auto armourMarker=reachableRawMarker(original,sim,armourTown,0x36);
    if(armourMarker.first<0) armourMarker=reachableRawMarker(original,sim,armourTown,0x37);
    require(armourMarker.first>=0,"no reachable Armour counter from actual town entry"); walkLocalTo(sim,armourTown,armourMarker.first,armourMarker.second);
    msg.clear(); require(shop.begin(original,armourTown,p,msg),"actual town Armour shop did not open");
    msg.clear(); require(!shop.handleKey('B',original,armourTown,p,sim,msg),"armour shop buy stage ended unexpectedly");
    msg.clear(); require(shop.handleKey('F',original,armourTown,p,sim,msg),"Reflect Armour purchase did not finish");
    p.readyArmour=5; require(p.armour[5]>0,"Reflect Armour purchase missing from inventory");

    // Return by normal Aircar movement to the Shuttle that was bought earlier,
    // X-it the Aircar on the Shuttle square, then Board the Shuttle normally.
    walkWorldTo(original,vehicles,overworld,p,p.worldObjects[(size_t)findObjectTile(p,0x11)].x(),p.worldObjects[(size_t)findObjectTile(p,0x11)].y());
    msg.clear();vehicles.exitCraft(original,p,false,msg);require(p.transport==0,"Aircar X-it failed before Shuttle boarding");
    msg.clear();vehicles.board(p,false,msg);require(p.transport==6,"normal Board command did not enter purchased Shuttle");
    require(p.shuttleStat0>=200 && p.shuttleStat1>0,"purchased Shuttle has unusable fuel/shield");
    SpaceSystem spaceSystem; SpaceState space;
    msg.clear(); spaceSystem.enter(space,p,msg); require(space.active,"space entry failed");
    // Dock the purchased Shuttle at the real home base, pay/refuel, and select the
    // existing 5000-fuel type-1 craft from the real left port.
    dockPort(spaceSystem,space,p,2,msg); require(space.choosingShip,"home base did not offer alternate ships");
    msg.clear(); require(spaceSystem.chooseShip(space,p,1,msg),"left-port ship selection failed"); require(liveCraft(space).type==1,"left port is not type-1 craft");
    undockRetro(spaceSystem,space,p,msg);
    int hostileVisited=0, refuels=1;
    std::set<std::pair<int,int>> cleared;
    while(p.spaceKills<20){
        // Find nearest uncleared hostile sector in the actual generated 7x7 universe.
        std::pair<int,int> best{-1,-1}; int bd=99;
        for(int x=0;x<7;++x)for(int y=0;y<7;++y){int n=space.sectors[(size_t)(x*7+y)].enemies;if(n<=0)continue;int d=std::abs(torusDelta(space.sectorX,x))+std::abs(torusDelta(space.sectorY,y));if(d<bd){bd=d;best={x,y};}}
        require(best.first>=0,"generated universe exhausted before Space Ace");
        // Keep a reserve to return home; if low, return and use the real 500p base service.
        int homeCost=(std::abs(torusDelta(best.first,3))+std::abs(torusDelta(best.second,3)))*100;
        if(liveCraft(space).fuel < bd*100 + homeCost + 1200){route(spaceSystem,space,p,3,3,msg);dockPort(spaceSystem,space,p,0,msg);++refuels;undockRetro(spaceSystem,space,p,msg);continue;}
        route(spaceSystem,space,p,best.first,best.second,msg); clearIfHostile(spaceSystem,space,p,msg);++hostileVisited;cleared.insert(best);
    }
    require(p.spaceKills>=20&&p.experience>=7000,"live generated universe did not reach Space Ace");
    // Return the type-1 craft to its real left port, switch back to the Shuttle at
    // the top port, undock it, then fly under normal top-view motion into Ultima.
    route(spaceSystem,space,p,3,3,msg); dockPort(spaceSystem,space,p,0,msg); require(space.choosingShip,"final dock did not offer shuttle");
    msg.clear(); require(spaceSystem.chooseShip(space,p,3,msg),"top-port Shuttle selection failed"); require(liveCraft(space).type==0,"top port is not Shuttle"); undockRetro(spaceSystem,space,p,msg);
    if(space.frontView){msg.clear();spaceSystem.toggleView(space,p,msg);}
    // Route clear of the base/star, then enter the world's <25 landing box around (60,75).
    driveTo(spaceSystem,space,p,0,-1,liveCraft(space).x,20,msg); driveTo(spaceSystem,space,p,-1,0,60,20,msg); setVel(spaceSystem,space,p,0,1,msg);
    int landGuard=0; while(space.active&&landGuard++<200){msg.clear();spaceSystem.updateFrame(space,p,msg);} require(!space.active,"normal Shuttle flight did not land on Ultima");
    require(p.transport==6,"landing discarded Shuttle state"); msg.clear();vehicles.exitCraft(original,p,false,msg);require(p.transport==0,"X-it after landing failed");
    std::cout<<"PASS generated 7x7 universe -> real base ship swap/refuel -> HyperJump traversal -> live aim/fire -> Space Ace -> Shuttle swap -> normal landing/X-it; kills="<<p.spaceKills<<", hostile sectors="<<hostileVisited<<", refuels="<<refuels<<"\\n";

    // M37: enter the real castle at its actual gate, kill the real Jester for the key,
    // unlock the randomized correct keyed door, reach the real Princess, then lead her back
    // to the west exit while actor/guard updates run after every command.
    const auto cit=std::find_if(original.locations().begin(),original.locations().end(),
                                [](const LocationInfo& l){return l.id==33;});
    require(cit!=original.locations().end(),"castle 33 missing");
    walkWorldTo(original,vehicles,overworld,p,cit->x,cit->y);
    // Deterministic regression fixture: seed 8 makes the first castle-entry key roll
    // select 0x3D (the Princess-room door). All movement/combat/AI after that still
    // consumes the ordinary shared DOS RNG. M37 audit separately covers the 0x3C reroll branch.
    sim.seed(8);
    LocalState castle;
    int castleEntries=0;
    for(;castleEntries<32;++castleEntries){
        castle=makeLocal(original,sim,*cit);
        require(castle.x==0&&castle.y==9,"unexpected castle 33 entry square");
        if(castleEntries==0){
            // Normal king donation before committing the crime: build a health reserve for
            // the guard pursuit that follows the Jester attack and possible key rerolls.
            auto km=reachableRawMarker(original,sim,castle,0x3E); require(km.first>=0,"castle 33 king marker unreachable");
            require(walkLiveTo(sim,castle,p,km.first,km.second,msg),"could not reach king before Princess rescue");
            msg.clear(); require(king.begin(original,castle,p,storeroomAllowance,msg),"castle 33 king donation dialogue did not open");
            msg.clear(); require(!king.handleKey('P',p,msg),"king pence stage ended early");
            int donate=std::min<int>(6000,p.coin); for(char ch:std::to_string(donate)){msg.clear();require(!king.handleKey(ch,p,msg),"king amount input ended early");}
            msg.clear(); require(king.handleKey('\r',p,msg),"king donation did not complete"); finishLocal(sim,castle,p,msg);
        }
        killLiveJester(sim,castle,p,msg);
        require(castle.alert&&castle.foundKey,"live Jester combat did not establish castle alert/key state");
        require(tryUnlockLiveDoor(original,sim,castle,p,msg),"neither real castle key marker opened its north door");
        if(princessRoomReachable(sim,castle)){ leadPrincessAndExit(sim,castle,p,msg); break; }
        // Original castle setup can legitimately choose the other keyed room. Leave by the
        // real gate and re-enter; setup rerolls 0x3C/0x3D and restores the actor template.
        leaveCastleWithoutPrincess(sim,castle,p,msg);
    }
    require(!castle.active,"Princess rescue did not leave castle-local mode");
    require(castleEntries<32,"randomized castle key never opened the Princess room");
    require(p.transports[7]>0,"princess did not grant Time Machine inventory");
    const int tmIndex=findObjectTile(p,0x12);
    require(tmIndex>0,"princess did not spawn Time Machine world object");
    const int tmx=p.worldObjects[static_cast<std::size_t>(tmIndex)].x();
    const int tmy=p.worldObjects[static_cast<std::size_t>(tmIndex)].y();
    walkWorldTo(original,vehicles,overworld,p,tmx,tmy);
    msg.clear(); vehicles.board(p,false,msg);
    require(p.transport==7,"Time Machine boarding failed");
    require(findObjectTile(p,0x12)<0,"boarded Time Machine world object was not consumed");
    std::cout<<"PASS live castle entry/re-entry -> Jester combat/key -> randomized Unlock -> Princess follow -> real gate rescue -> Time Machine; entries="<<(castleEntries+1)<<"\n";

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
    std::cout<<"M38 campaign smoke PASS\n";
    return 0;
}
