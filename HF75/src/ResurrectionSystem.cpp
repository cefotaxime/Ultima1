#include "ResurrectionSystem.h"
#include "OverworldSimulation.h"
#include <algorithm>

bool ResurrectionSystem::occupied(const PlayerSave& p,int x,int y) const{
    for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i){
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        if(!o.empty() && static_cast<int>(o.x())==x && static_cast<int>(o.y())==y) return true;
    }
    return false;
}

void ResurrectionSystem::removeAllMonsters(PlayerSave& p) const{
    // OUT.EXE resurrection invokes the quadrant monster cleanup routine for
    // quadrants 0,1,2,3. Equivalent result: remove every overworld monster,
    // preserving all vehicles/other world objects and stable record order.
    std::array<WorldObject,OriginalData::WORLD_OBJECT_COUNT> kept{};
    kept[0]=p.worldObjects[0];
    int out=1;
    for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT && out<OriginalData::WORLD_OBJECT_COUNT;++i){
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        if(o.empty()) continue;
        if(OverworldSimulation::isMonsterTile(static_cast<int>(o.tile()))) continue;
        kept[static_cast<std::size_t>(out++)]=o;
    }
    p.worldObjects=kept;
}

void ResurrectionSystem::beginAnimated(PlayerSave& p) const{
    // OUT.EXE file 0xC67C: Coin, Food and Hits are zeroed before the skull/death
    // presentation. Hits and Food are subsequently counted back up on screen.
    p.food=0;
    p.hits=0;
    p.coin=0;
}

void ResurrectionSystem::afterHitsAnimated(PlayerSave& p) const{
    // OUT.EXE 0xC720..0xC741: all four overworld monster quadrants are cleaned
    // after Hits reaches 99 and before the Food counter begins.
    removeAllMonsters(p);
}

void ResurrectionSystem::finishCommon(PlayerSave& p) const{
    const int oldX=static_cast<int>(p.x), oldY=static_cast<int>(p.y);
    const int oldQuadrant=quadrant(oldX,oldY);

    if(p.transport!=0){
        const int t=static_cast<int>(p.transport);
        if(t>=0 && t<static_cast<int>(p.transports.size()) && p.transports[static_cast<std::size_t>(t)]>0)
            --p.transports[static_cast<std::size_t>(t)];
        p.transport=0;
        p.worldObjects[0].w[0]=10;
    }

    // The original explicitly clears every non-hands weapon slot on death.
    for(int i=1;i<16;++i) p.weapons[static_cast<std::size_t>(i)]=0;
    p.readyWeapon=0; p.readyArmour=0; p.readySpell=0;

    // OUT.EXE file 0x8E20: choose a nearby grass/forest square in the same
    // 84x78 quadrant. Out-of-interior candidate axes fall back to the old axis.
    int nx=oldX,ny=oldY,terrain=original_.terrainAt(oldX,oldY);
    for(int guard=0;guard<8192;++guard){
        int cx=oldX + rng_.randomRange(3,23)-13;
        int cy=oldY + rng_.randomRange(3,23)-13;
        if(cx<2 || cx>165) cx=oldX;
        if(cy<2 || cy>153) cy=oldY;
        if(quadrant(cx,cy)!=oldQuadrant) continue;
        if(occupied(p,cx,cy)) continue;
        const int q=original_.terrainAt(cx,cy);
        if(q!=1 && q!=2) continue;
        nx=cx; ny=cy; terrain=q; break;
    }
    p.x=static_cast<std::uint16_t>(nx); p.y=static_cast<std::uint16_t>(ny);
    p.worldObjects[0].w[0]=10;
    p.worldObjects[0].w[1]=static_cast<std::uint16_t>(terrain);
    p.worldObjects[0].w[2]=p.x; p.worldObjects[0].w[3]=p.y;
}

void ResurrectionSystem::finishAnimated(PlayerSave& p) const{
    finishCommon(p);
}

std::vector<std::string> ResurrectionSystem::resurrect(PlayerSave& p) const{
    std::vector<std::string> m;
    if(!needed(p)) return m;

    beginAnimated(p);
    m.push_back(p.name+"님, 당신은 사망하였습니다.");
    m.push_back("부활을 시도합니다!");
    p.hits=99;
    afterHitsAnimated(p);
    p.food=99;
    finishAnimated(p);
    return m;
}
