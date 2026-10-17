#pragma once

#include <utility>
#include <vector>

// OUT.EXE EF3 does NOT insert a standalone delay between accepting the attack
// direction and drawing the first physical-tile-50 attack effect.  The first
// cell is drawn immediately.  While that first cell is still on screen the
// routine either executes the synchronous PC-speaker call (distance==1) or,
// for later cells, delay(2).  The SDL host therefore must not add a separate
// pre-effect "direction hold".

// OUT.EXE EF3 waits delay(2) between projectile cells after the first draw.
constexpr unsigned WORLD_PROJECTILE_DOS_TICKS=2;

// Build the original EF3-style cardinal projectile path in *screen-cell deltas*.
// queryCell(dx,dy) returns the OUT physical/terrain code at that offset.
// EF3 keeps travelling through ordinary terrain/objects, but stops after drawing
// a cell whose query result is a mountain (3) or a physical object >=19.
template<class QueryCell>
inline std::vector<std::pair<int,int>> buildWorldProjectileVisualPath(
    int dx,int dy,int range,QueryCell queryCell){
    std::vector<std::pair<int,int>> path;
    if(range<=0 || (dx==0 && dy==0)) return path;
    for(int dist=1;dist<=range;++dist){
        const int sx=dx*dist;
        const int sy=dy*dist;
        path.push_back({sx,sy});
        const int q=queryCell(sx,sy);
        if(q>=19 || q==3) break;
    }
    return path;
}