#include "VehicleSystem.h"
#include "KoreanText.h"
#include <algorithm>

namespace {
constexpr int PLAYER_TILE = 10;
}

const char* VehicleSystem::transportName(int t){
    return 운송수단_이름_한글(t);
}

int VehicleSystem::transportFromTile(int tile){
    // OUT.EXE Board 0x13DC..0x1420: accepted physical tiles 0x0B..0x12,
    // internal transport = tile-10, with the gap at physical tile 0x0F removed.
    if(tile<0x0B || tile>0x12) return 0;
    int t=tile-0x0A;
    if(t>4) --t;
    return t;
}

int VehicleSystem::lastObject(const PlayerSave& p){
    for(int i=OriginalData::WORLD_OBJECT_COUNT-1;i>0;--i)
        if(!p.worldObjects[static_cast<std::size_t>(i)].empty()) return i;
    return 0;
}

int VehicleSystem::objectAt(const PlayerSave& p,int x,int y){
    const int last=lastObject(p);
    for(int i=1;i<=last;++i){
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        if(!o.empty() && static_cast<int>(o.x())==x && static_cast<int>(o.y())==y) return i;
    }
    return -1;
}

void VehicleSystem::eraseObject(PlayerSave& p,int index){
    // OUT.EXE b78e removes an object by shifting every later 16-byte record left.
    const int last=lastObject(p);
    if(index<=0 || index>last) return;
    for(int i=index;i<last;++i)
        p.worldObjects[static_cast<std::size_t>(i)] = p.worldObjects[static_cast<std::size_t>(i+1)];
    p.worldObjects[static_cast<std::size_t>(last)].w.fill(0);
}

bool VehicleSystem::appendObject(PlayerSave& p,const WorldObject& o){
    const int last=lastObject(p);
    if(last>=OriginalData::WORLD_OBJECT_COUNT-1) return false;
    p.worldObjects[static_cast<std::size_t>(last+1)]=o;
    return true;
}

bool VehicleSystem::board(PlayerSave& p,bool localActive,std::vector<std::string>& m) const {
    // OUT.EXE load-image 0x138D.
    if(localActive){ m.push_back("타기?"); return true; }
    if(p.transport!=0){ m.push_back("먼저 내려야 합니다!"); return true; }
    const int oi=objectAt(p,p.x,p.y);
    if(oi<0){ m.push_back("무엇을 탑니까?"); return true; }
    const int tile=static_cast<int>(p.worldObjects[static_cast<std::size_t>(oi)].tile());
    const int t=transportFromTile(tile);
    if(t==0){ m.push_back("무엇을 탑니까?"); return true; }

    if(t==6){
        // OUT.EXE removes the Shuttle object, makes it the player's current
        // transport, saves INUSE.U1, then chains to SPACE.EXE. M15 performs
        // the same state handoff internally instead of spawning another EXE.
        m.push_back("우주 왕복선에 탑승합니다!");
        p.transport=6;
        p.worldObjects[0].w[0]=static_cast<std::uint16_t>(tile);
        eraseObject(p,oi);
        return true;
    }
    if(t==7){
        // OUT.EXE load-image 0x118D..0x12ED: boarding happens before the
        // four-gem launch test.  The Time Machine object is removed and the
        // player transport/tile are updated even when one or more gems are
        // missing.  Only the subsequent MONDAIN.EXE handoff is conditional.
        m.push_back("타임머신 타기");
        p.transport=7;
        p.worldObjects[0].w[0]=static_cast<std::uint16_t>(tile);
        eraseObject(p,oi);
        const bool fourGems = std::all_of(p.gems.begin(),p.gems.end(),[](std::uint16_t n){return n!=0;});
        // M280: OUT.EXE 0x14A1 does not print the four-gem failure text
        // into the command log. It opens the shared 0x23C2 framed popup and
        // writes three rows there; main.cpp owns that modal presentation.
        (void)fourGems;
        return true;
    }

    switch(t){
        case 1: m.push_back("말을 탑니다!"); break;
        case 2: m.push_back("마차를 탑니다!"); break;
        case 3: m.push_back("뗏목을 탑니다!"); break;
        case 4: m.push_back("승선합니다!"); break;
        case 5: m.push_back("비행선에 탑승합니다!"); break;
        default: m.push_back(std::string(transportName(t))+"에 탑승합니다!"); break;
    }
    p.transport=static_cast<std::uint16_t>(t);
    p.worldObjects[0].w[0]=static_cast<std::uint16_t>(tile);
    eraseObject(p,oi);
    return true;
}

bool VehicleSystem::anyLandWithin2(const OriginalData& original,int x,int y){
    // OUT.EXE X-it 0x7FCD..0x8001 scans the full 5x5 square and calls the
    // raw map lookup; any non-water map value makes disembarking possible.
    for(int dx=-2;dx<=2;++dx) for(int dy=-2;dy<=2;++dy){
        const int xx=x+dx, yy=y+dy;
        if(xx<0||yy<0||xx>=OriginalData::MAP_W||yy>=OriginalData::MAP_H) continue;
        if(original.terrainAt(xx,yy)!=0) return true;
    }
    return false;
}

bool VehicleSystem::exitCraft(const OriginalData& original,PlayerSave& p,bool localActive,
                              std::vector<std::string>& m) const {
    // OUT.EXE load-image 0x7F6B.
    if(localActive){ m.push_back("내리기?"); return true; }
    if(p.transport==0){ m.push_back("무엇에서 내립니까?"); return true; }

    const int terrain=original.terrainAt(p.x,p.y);

    // M219 / OUT.EXE 0x7FAE..0x7FC4: X-it does NOT inspect only the raw
    // terrain under the player.  It calls the same object-aware world-cell
    // query used by movement (C4AF), starting at world-object index 1.
    // Therefore any live object already occupying the current square replaces
    // the raw terrain value.  In particular, if another horse/cart/raft/ship/
    // aircar/shuttle/time-machine is already here, its physical tile is >2 and
    // X-it must fail instead of appending a second transport on the same cell.
    int cell=terrain;
    const int occupied=objectAt(p,p.x,p.y);
    if(occupied>=0)
        cell=static_cast<int>(p.worldObjects[static_cast<std::size_t>(occupied)].tile());

    // OUT.EXE 0x8001..0x8021: X-it succeeds only when BOTH conditions hold:
    //   1) the object-aware current cell is class 0..2 (water/grass/forest), and
    //   2) the raw 5x5 map square centred on the player contains at least one
    //      non-water cell.  Thus occupied cells and castles/towns/dungeons/
    //      landmarks/mountains are never valid X-it cells, while open water is
    //      valid only within two cells (Chebyshev distance) of non-water terrain.
    if(cell>2 || !anyLandWithin2(original,p.x,p.y)){
        m.push_back("여기서 내릴 수는 없습니다!");
        return true;
    }

    const int craftTile=static_cast<int>(p.worldObjects[0].tile());
    WorldObject o; o.w.fill(0);
    o.w[0]=static_cast<std::uint16_t>(craftTile);
    // The original object append receives the current map-class byte as word1.
    o.w[1]=static_cast<std::uint16_t>(std::clamp(terrain,0,7));
    o.w[2]=p.x; o.w[3]=p.y;
    if(!appendObject(p,o)){
        m.push_back("여기서 내릴 수는 없습니다!");
        return true;
    }
    p.transport=0;
    p.worldObjects[0].w[0]=PLAYER_TILE;
    m.push_back("내리기");
    return true;
}

bool VehicleSystem::mayMove(const OriginalData& original,const PlayerSave& p,int nx,int ny,
                            std::vector<std::string>& m) const {
    // OUT.EXE 0x06F7..0x071A does NOT test the raw terrain first.  It calls the
    // object-aware world-cell query (0xC4AF): a live world object at the target
    // coordinate replaces the underlying map value.  This is essential for
    // boarding water craft: a Raft/Frigate on water therefore yields physical
    // tile 13/14 instead of water class 0, so a pedestrian may step onto the
    // craft square and then issue Board.
    int cell=original.terrainAt(nx,ny);
    const int oi=objectAt(p,nx,ny);
    if(oi>=0) cell=static_cast<int>(p.worldObjects[static_cast<std::size_t>(oi)].tile());

    const int t=static_cast<int>(p.transport);

    // 0x0728..0x07CA handles an encountered roaming-monster object before the
    // terrain/transport tests below.  main.cpp normally emits the localized
    // monster-specific wording first; retaining the block here also keeps
    // headless/pathfinding callers faithful to the combined-cell query.
    if(cell>=0x13 && cell<=0x2F){
        m.push_back("괴수 때문에 길이 막혔습니다!");
        return false;
    }

    // Exact ordering from OUT.EXE load-image 0x07CD..0x086E.  Transport-object
    // tiles 0x0B..0x12 match none of these terrain cases and intentionally fall
    // through to success, regardless of the water beneath them.
    if(cell==3){ m.push_back("산을 통과할 수 없습니다!"); return false; }
    if(cell==0){
        if(t==3 || t==4 || t==5) return true;
        m.push_back("물 위를 걸을 수 없습니다!"); return false;
    }
    if(cell==2 && t==5){ m.push_back("비행선은 숲을 통과할 수 없습니다!"); return false; }
    if(t==4 && cell!=0){ m.push_back("범선은 바다를 좋아합니다!"); return false; }
    if(t==3 && cell!=0){ m.push_back("뗏목은 바다를 좋아합니다!"); return false; }
    if(t>=6){ m.push_back("육지에서는 이동할 수 없습니다!"); return false; }
    return true;
}