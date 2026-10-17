#include "SpaceSystem.h"
#include "PixelFont.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace {
constexpr int VIEW_X=16, VIEW_Y=16, VIEW_W=608, VIEW_H=288;
constexpr SDL_Color DIM{85,85,85,255};
constexpr SDL_Color BLUE{85,85,255,255};
constexpr SDL_Color RED{255,85,85,255};
constexpr SDL_Color CYAN{85,255,255,255};

// SPACE.EXE DS:0AEE and DS:0B06. Index is port*3 + shipType.
constexpr int PORT_DX[12]={-17,-15,-15, 18,18,18, 0,1,2, 0,1,2};
constexpr int PORT_DY[12]={  1,  1,  1,  1, 1,1,-17,-16,-16,18,17,17};

// SPACE.EXE DS:01F6/0204/0212/0220.  These are the exact per-stage
// front-view enemy hitbox edges relative to DS:0C92/0C94.
constexpr int ENEMY_X_MAX[7]={5,6,8,10,12,16,18};
constexpr int ENEMY_Y_MAX[7]={4,6,8,10,12,14,18};
constexpr int ENEMY_X_MIN[7]={1,1,1,1,0,0,0};
constexpr int ENEMY_Y_MIN[7]={5,5,4,4,3,2,1};

int signum(int v){ return (v>0)?1:((v<0)?-1:0); }

constexpr std::array<SDL_Color,16> EGA={{{0,0,0,255},{0,0,170,255},{0,170,0,255},{0,170,170,255},{170,0,0,255},{170,0,170,255},{170,85,0,255},{170,170,170,255},{85,85,85,255},{85,85,255,255},{85,255,85,255},{85,255,255,255},{255,85,85,255},{255,85,255,255},{255,255,85,255},{255,255,255,255}}};
inline void logicalPixel(SDL_Renderer* r,int x,int y,SDL_Color c){SDL_Rect q{x*2,y*2,2,2};SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);SDL_RenderFillRect(r,&q);}
void logicalLine(SDL_Renderer* r,int x0,int y0,int x1,int y1,SDL_Color c){
    const int dx=std::abs(x1-x0), sx=x0<x1?1:-1;
    const int dy=-std::abs(y1-y0), sy=y0<y1?1:-1;
    int err=dx+dy;
    for(;;){ logicalPixel(r,x0,y0,c); if(x0==x1&&y0==y1)break; const int e2=2*err; if(e2>=dy){err+=dy;x0+=sx;} if(e2<=dx){err+=dx;y0+=sy;} }
}
void logicalFill(SDL_Renderer* r,int x0,int y0,int x1,int y1,SDL_Color c){
    if(x0>x1)std::swap(x0,x1); if(y0>y1)std::swap(y0,y1);
    SDL_Rect q{x0*2,y0*2,(x1-x0+1)*2,(y1-y0+1)*2}; SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); SDL_RenderFillRect(r,&q);
}
std::vector<std::uint8_t> readAll(const std::string& path){std::ifstream f(path,std::ios::binary);if(!f)return {};return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
std::uint16_t le16(const std::vector<std::uint8_t>& b,std::size_t o){return static_cast<std::uint16_t>(b[o]|(static_cast<std::uint16_t>(b[o+1])<<8));}

void line(SDL_Renderer* r,int x0,int y0,int x1,int y1,SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    const int dx=std::abs(x1-x0), sx=x0<x1?1:-1;
    const int dy=-std::abs(y1-y0), sy=y0<y1?1:-1;
    int err=dx+dy;
    for(;;){
        SDL_RenderDrawPoint(r,x0,y0);
        if(x0==x1&&y0==y1) break;
        const int e2=2*err;
        if(e2>=dy){err+=dy;x0+=sx;}
        if(e2<=dx){err+=dx;y0+=sy;}
    }
}
void box(SDL_Renderer* r,int x,int y,int w,int h,SDL_Color c){
    SDL_Rect q{x,y,w,h}; SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); SDL_RenderDrawRect(r,&q);
}

struct FlamePoint { int x,y; bool hot; };
// SPACE.EXE top-view thrust exhaust raster, recovered from the three
// ship-specific draw routines (file 0x2A1F/0x30C1/0x2D7B). Coordinates are
// relative to the craft sprite top-left; heading order is 0=E,1=W,2=S,3=N.
constexpr FlamePoint SHUTTLE_FLAME[4][12]={
    {{19,4,false},{19,11,false},{19,5,false},{19,12,false},{19,6,false},{19,13,false},{20,5,true},{20,6,true},{20,11,true},{20,12,true},{21,6,false},{21,11,false}},
    {{-1,4,false},{-1,11,false},{-1,5,false},{-1,12,false},{-1,6,false},{-1,13,false},{-2,5,true},{-2,6,true},{-2,11,true},{-2,12,true},{-3,6,false},{-3,11,false}},
    {{5,19,false},{12,19,false},{6,19,false},{13,19,false},{7,19,false},{14,19,false},{6,20,true},{7,20,true},{12,20,true},{13,20,true},{7,21,false},{12,21,false}},
    {{5,-1,false},{12,-1,false},{6,-1,false},{13,-1,false},{7,-1,false},{14,-1,false},{6,-2,true},{7,-2,true},{12,-2,true},{13,-2,true},{7,-3,false},{12,-3,false}}
};
constexpr FlamePoint SHIP1_FLAME[4][12]={
    {{17,7,false},{17,9,false},{18,8,false},{17,8,false},{17,10,false},{18,9,false},{19,6,false},{19,7,false},{19,8,false},{19,9,false},{19,10,false},{19,11,false}},
    {{-1,7,false},{-1,9,false},{-2,8,false},{-1,8,false},{-1,10,false},{-2,9,false},{-3,6,false},{-3,7,false},{-3,8,false},{-3,9,false},{-3,10,false},{-3,11,false}},
    {{7,18,false},{9,18,false},{8,19,false},{8,18,false},{10,18,false},{9,19,false},{6,20,false},{7,20,false},{8,20,false},{9,20,false},{10,20,false},{11,20,false}},
    {{7,0,false},{9,0,false},{8,-1,false},{8,0,false},{10,0,false},{9,-1,false},{6,-2,false},{7,-2,false},{8,-2,false},{9,-2,false},{10,-2,false},{11,-2,false}}
};
constexpr FlamePoint SHIP2_FLAME[4][14]={
    {{17,1,false},{17,2,false},{17,15,false},{17,16,false},{18,1,false},{18,2,false},{18,15,false},{18,16,false},{17,7,false},{17,8,false},{17,9,false},{17,10,false},{18,6,false},{18,11,false}},
    {{-1,1,false},{-1,2,false},{-1,15,false},{-1,16,false},{-2,1,false},{-2,2,false},{-2,15,false},{-2,16,false},{-1,7,false},{-1,8,false},{-1,9,false},{-1,10,false},{-2,6,false},{-2,11,false}},
    {{0,18,false},{0,19,false},{14,18,false},{14,19,false},{1,18,false},{1,19,false},{15,18,false},{15,19,false},{6,18,false},{7,18,false},{8,18,false},{9,18,false},{5,19,false},{10,19,false}},
    {{0,0,false},{0,-1,false},{14,0,false},{14,-1,false},{1,0,false},{1,-1,false},{15,0,false},{15,-1,false},{6,0,false},{7,0,false},{8,0,false},{9,0,false},{5,-1,false},{10,-1,false}}
};
}



bool SpaceSystem::loadGraphics(const std::string& dataDir,std::string& error){
    auto space=readAll(dataDir+"/EGASPACE.BIN"), fight=readAll(dataDir+"/EGAFIGHT.BIN"), exe=readAll(dataDir+"/SPACE.EXE");
    if(space.size()!=7296){error="EGASPACE.BIN must be 7,296 bytes";return false;} if(fight.size()!=3192){error="EGAFIGHT.BIN must be 3,192 bytes";return false;} if(exe.size()!=39824){error="SPACE.EXE must be 39,824 bytes";return false;}
    for(int i=0;i<12;++i)std::copy_n(space.begin()+i*608,76,topMasks_[i].begin());
    for(int i=0;i<7;++i)std::copy_n(fight.begin()+i*456,57,fightMasks_[i].begin());
    constexpr std::size_t DS=0x74D0; for(std::size_t i=0;i<starRaster_.size();++i)starRaster_[i]=le16(exe,DS + 0x022E + i*2); for(std::size_t i=0;i<baseRaster_.size();++i)baseRaster_[i]=le16(exe,DS + 0x04DA + i*2); for(std::size_t i=0;i<worldRaster_.size();++i)worldRaster_[i]=le16(exe,DS + 0x07FA + i*2);
    graphicsLoaded_=true; return true;
}
bool SpaceSystem::topSpritePixel(int type,int heading,int px,int py) const{if(!graphicsLoaded_||type<0||type>2||heading<0||heading>3||px<0||px>=32||py<0||py>=19)return false;const auto&m=topMasks_[type*4+heading];return (m[py*4+(px>>3)]&(0x80u>>(px&7)))!=0;}
bool SpaceSystem::fightSpritePixel(int stage,int px,int py) const{if(!graphicsLoaded_||stage<0||stage>6||px<0||px>=24||py<0||py>=19)return false;const auto&m=fightMasks_[stage];return (m[py*3+(px>>3)]&(0x80u>>(px&7)))!=0;}
bool SpaceSystem::topCollision(const SpaceState&s,int movingSlot,int nx,int ny) const{if(!graphicsLoaded_)return false;const auto&q=sector(s);const auto&me=q.craft[movingSlot];auto occ=[&](int x,int y){if(q.starX){int rx=x-q.starX,ry=y-q.starY;if(rx>=0&&rx<19&&ry>=0&&ry<18&&starRaster_[ry*19+rx])return true;}if(q.baseX){int rx=x-q.baseX,ry=y-q.baseY;if(rx>=0&&rx<20&&ry>=0&&ry<20&&baseRaster_[ry*20+rx])return true;}if(s.sectorX==3&&s.sectorY==3){int rx=x-60,ry=y-75;if(rx>=0&&rx<21&&ry>=0&&ry<18&&worldRaster_[ry*21+rx])return true;}for(int i=0;i<4;++i){if(i==movingSlot)continue;const auto&z=q.craft[i];if(!z.active())continue;int rx=x-z.x,ry=y-z.y;if(rx>=0&&rx<32&&ry>=0&&ry<19&&topSpritePixel(z.type,z.heading,rx,ry))return true;}return false;};for(int y=0;y<19;++y)for(int x=0;x<32;++x)if(topSpritePixel(me.type,me.heading,x,y)&&occ(nx+x,ny+y))return true;return false;}
void SpaceSystem::drawTopExact(SDL_Renderer*r,const SpaceState&s) const{
    if(!graphicsLoaded_)return;
    const auto&q=sector(s);
    if(q.starX)for(int y=0;y<18;++y)for(int x=0;x<19;++x){auto v=starRaster_[y*19+x];if(v==1)logicalPixel(r,q.starX+x,q.starY+y,EGA[14]);else if(v==2)logicalPixel(r,q.starX+x,q.starY+y,EGA[12]);}
    if(q.baseX)for(int y=0;y<20;++y)for(int x=0;x<20;++x)if(baseRaster_[y*20+x])logicalPixel(r,q.baseX+x,q.baseY+y,EGA[15]);
    if(s.sectorX==3&&s.sectorY==3)for(int y=0;y<18;++y)for(int x=0;x<21;++x){auto v=worldRaster_[y*21+x];int c=v==1?1:v==2?10:v==3?1:v==4?7:-1;if(c>=0)logicalPixel(r,60+x,75+y,EGA[c]);}

    // SPACE.EXE 0x3309: while DS:012C is non-zero, draw the type-specific
    // exhaust behind the currently controlled ship.  Type 0 uses the original
    // two-colour inner/outer pattern; the other two routines use one colour.
    if(s.thrustCounter>0&&s.currentSlot>=0&&s.currentSlot<4){
        const auto& z=q.craft[static_cast<std::size_t>(s.currentSlot)];
        const int h=std::clamp(z.heading,0,3);
        auto flamePixel=[&](const FlamePoint& pt){
            const int px=z.x+pt.x,py=z.y+pt.y;
            if(px>20&&px<300&&py>18&&py<141) logicalPixel(r,px,py,pt.hot?EGA[15]:EGA[12]);
        };
        if(z.type==0) for(const auto& pt:SHUTTLE_FLAME[h]) flamePixel(pt);
        else if(z.type==1) for(const auto& pt:SHIP1_FLAME[h]) flamePixel(pt);
        else if(z.type==2) for(const auto& pt:SHIP2_FLAME[h]) flamePixel(pt);
    }

    for(int i=0;i<4;++i){
        if(s.collisionFlashFrames>0&&i==s.currentSlot)continue;
        const auto&z=q.craft[static_cast<std::size_t>(i)];if(!z.active())continue;
        for(int y=0;y<19;++y)for(int x=0;x<32;++x)if(topSpritePixel(z.type,z.heading,x,y))logicalPixel(r,z.x+x,z.y+y,EGA[15]);
    }
    if(s.collisionFlashFrames>0&&s.currentSlot>=0&&s.currentSlot<4){
        const auto&me=q.craft[static_cast<std::size_t>(s.currentSlot)];
        auto backgroundIndex=[&](int px,int py){
            int c=0;
            if(q.starX){int x=px-q.starX,y=py-q.starY;if(x>=0&&x<19&&y>=0&&y<18){auto v=starRaster_[y*19+x];if(v==1)c=14;else if(v==2)c=12;}}
            if(q.baseX){int x=px-q.baseX,y=py-q.baseY;if(x>=0&&x<20&&y>=0&&y<20&&baseRaster_[y*20+x])c=15;}
            if(s.sectorX==3&&s.sectorY==3){int x=px-60,y=py-75;if(x>=0&&x<21&&y>=0&&y<18){auto v=worldRaster_[y*21+x];if(v==1||v==3)c=1;else if(v==2)c=10;else if(v==4)c=7;}}
            for(int i=0;i<4;++i){if(i==s.currentSlot)continue;const auto&z=q.craft[static_cast<std::size_t>(i)];if(!z.active())continue;int x=px-z.x,y=py-z.y;if(x>=0&&x<32&&y>=0&&y<19&&topSpritePixel(z.type,z.heading,x,y))c^=15;}
            return c;
        };
        // SPACE.EXE top blitter file 0x62E0 executes VGA/EGA XOR with the
        // foreground mask. Preserve the transient palette-index XOR result.
        for(int y=0;y<19;++y)for(int x=0;x<32;++x)if(topSpritePixel(me.type,me.heading,x,y)){
            const int px=s.collisionX+x,py=s.collisionY+y;logicalPixel(r,px,py,EGA[backgroundIndex(px,py)^15]);
        }
    }
}
void SpaceSystem::drawSpaceFrame(SDL_Renderer*r) const{
    // SPACE.EXE file 0x25F3: exact 320x200 logical viewport frame.
    logicalFill(r,8,8,311,15,EGA[7]);
    logicalFill(r,8,144,311,151,EGA[7]);
    logicalFill(r,8,16,15,144,EGA[7]);
    logicalFill(r,304,16,311,144,EGA[7]);
    for(int i=1;i<=19;++i){const int x=i*15+10; logicalPixel(r,x,10,EGA[0]);logicalPixel(r,x,13,EGA[0]);logicalPixel(r,x,146,EGA[0]);logicalPixel(r,x,149,EGA[0]);}
    for(int i=1;i<=9;++i){const int y=i*15+3; logicalPixel(r,10,y,EGA[0]);logicalPixel(r,13,y,EGA[0]);logicalPixel(r,306,y,EGA[0]);logicalPixel(r,309,y,EGA[0]);}
}

void SpaceSystem::drawSectorScan(SDL_Renderer*r,const SpaceState&s) const{
    // SPACE.EXE 0x16E5..0x183B.  The scan is a blocking popup.  0x11BC first
    // clears (31,23)..(287,127), then 0x1162 draws the blue inner popup frame
    // with edges x=34/284, y=26/124.  The 7x7 map itself is white.
    logicalFill(r,31,23,287,127,EGA[0]);
    logicalLine(r,34,26,284,26,EGA[1]);
    logicalLine(r,284,26,284,124,EGA[1]);
    logicalLine(r,34,124,284,124,EGA[1]);
    logicalLine(r,34,26,34,124,EGA[1]);

    logicalLine(r,130,44,200,44,EGA[15]); logicalLine(r,200,44,200,115,EGA[15]);
    logicalLine(r,130,115,200,115,EGA[15]); logicalLine(r,130,44,130,115,EGA[15]);
    auto enemy=[&](int x,int y){for(int i=0;i<2;++i){logicalLine(r,x+i,y,x+i,y+7,EGA[15]);logicalLine(r,x+5+i,y,x+5+i,y+7,EGA[15]);logicalLine(r,x,y+3+i,x+6,y+3+i,EGA[15]);}};
    auto base=[&](int x,int y){for(int i=0;i<2;++i){logicalLine(r,x+3+i,y,x+3+i,y+7,EGA[15]);logicalLine(r,x,y+3+i,x+7,y+3+i,EGA[15]);}};
    auto star=[&](int x,int y){for(int i=0;i<3;++i){logicalLine(r,x+3-i,y+i+1,x+3+i,y+i+1,EGA[15]);logicalLine(r,x+3-i,y+6-i,x+3+i,y+6-i,EGA[15]);}};
    auto empty=[&](int x,int y){logicalPixel(r,x+1,y+1,EGA[15]);logicalPixel(r,x+3,y+3,EGA[12]);logicalPixel(r,x+6,y+2,EGA[14]);logicalPixel(r,x+4,y+5,EGA[15]);logicalPixel(r,x+2,y+6,EGA[14]);};

    // SPACE.EXE 0x175E..0x1787 initializes x=0 as the OUTER loop and y=0
    // as the INNER loop, then calls 0x162B for each cell.  On DOS/EGA those
    // direct drawing calls are visible as a left-to-right column sweep.  A
    // buffered SDL frame would otherwise collapse all 49 cells into one
    // presentation, so expose one original x-column per presentation quantum.
    // There is no source delay between columns; SECTOR_SCAN_COLUMN_MS is only
    // the host-display granularity used to preserve the recovered draw order.
    const std::uint32_t elapsed=SDL_GetTicks()-s.sectorScanStartMs;
    const std::uint32_t revealDuration=7u*SpaceState::SECTOR_SCAN_COLUMN_MS;
    const int visibleColumns=std::min(7,1+static_cast<int>(elapsed/SpaceState::SECTOR_SCAN_COLUMN_MS));

    // After the initial 7-column draw completes, 0x17C7/0x180C alternates the
    // current sector glyph itself at 7 BIOS INT 1Ch ticks per phase.  There is
    // no separate cursor rectangle.
    constexpr std::uint32_t BIOS_7_TICKS_MS=385;
    const std::uint32_t blinkElapsed=(elapsed>revealDuration)?(elapsed-revealDuration):0u;
    const bool revealComplete=elapsed>=revealDuration;
    const bool showCurrent=!revealComplete || ((blinkElapsed/BIOS_7_TICKS_MS)&1u)==0u;
    for(int x=0;x<visibleColumns;++x)for(int y=0;y<7;++y){
        if(x==s.sectorX && y==s.sectorY && !showCurrent) continue;
        const auto&q=s.sectors[static_cast<std::size_t>(sectorIndex(x,y))];
        const int px=132+x*10,py=46+y*10;
        if(q.enemies)enemy(px,py);else if(q.baseX)base(px,py);else if(q.starX)star(px,py);else empty(px,py);
    }
}

void SpaceSystem::drawFightExact(SDL_Renderer*r,const SpaceState&s,SDL_Color white) const{
    if(!graphicsLoaded_)return;
    // SPACE.EXE 0x1E2A: current moving stars. Mode 1 retains every prior point.
    for(const auto&pt:s.starTrail) if(pt.first>20&&pt.first<300&&pt.second>20&&pt.second<140) logicalPixel(r,pt.first,pt.second,white);
    if(s.starsInitialized)for(int i=0;i<40;++i){int x=s.crosshairX+s.starX[i],y=s.crosshairY+s.starY[i];if(x>20&&x<300&&y>20&&y<140)logicalPixel(r,x,y,white);}
    if(s.hyperPhase==0 && s.enemyX){
        // Apple 1986 SPA: eight draw/erase pulses with a static star field.
        // First phase draws, second phase erases; eight cycles ~512 ms.
        const bool blinking=s.killBlinkUntilMs!=0 &&
            static_cast<std::int32_t>(s.killBlinkUntilMs-SDL_GetTicks())>0;
        const bool visible=!blinking ||
            ((SDL_GetTicks()-s.killBlinkStartMs)/32u)%2u==0u;
        if(visible){
            int st=std::clamp(s.enemySprite,0,6);
            for(int y=0;y<19;++y)for(int x=0;x<24;++x)
                if(fightSpritePixel(st,x,y))logicalPixel(r,s.enemyX+x,s.enemyY+y,white);
        }
    }
    if(s.playerShotUntilMs!=0 &&
       static_cast<std::int32_t>(s.playerShotUntilMs-SDL_GetTicks())>0){
        // Apple II SPA $97BE..$97D5 fires from (32,127) / (224,127)
        // in its 256-wide front-view coordinate system. Recenter and scale
        // to inset positions at the bottom edge of the DOS viewport (Y=143).
        logicalLine(r,40,143,s.playerShotX,s.playerShotY,EGA[12]);
        logicalLine(r,280,143,s.playerShotX,s.playerShotY,EGA[12]);
    }
    // SPACE.EXE 0x1DE9: exact 5-pixel horizontal/vertical gunsight.
    logicalLine(r,s.crosshairX-2,s.crosshairY,s.crosshairX+2,s.crosshairY,white);
    logicalLine(r,s.crosshairX,s.crosshairY-2,s.crosshairX,s.crosshairY+2,white);
    if(s.alienShotFrames>0) logicalLine(r,s.alienShotX0,s.alienShotY0,s.alienShotX1,s.alienShotY1,EGA[12]);
}


int SpaceSystem::wrap7(int v){ v%=7; if(v<0)v+=7; return v; }
int SpaceSystem::clampVelocity(int v){ return std::clamp(v,-9,9); }
int SpaceSystem::oppositeHeading(int h){ static constexpr int M[4]={1,0,3,2}; return M[std::clamp(h,0,3)]; }
int SpaceSystem::portX(int baseX,int port,int type){ return baseX+PORT_DX[std::clamp(port,0,3)*3+std::clamp(type,0,2)]; }
int SpaceSystem::portY(int baseY,int port,int type){ return baseY+PORT_DY[std::clamp(port,0,3)*3+std::clamp(type,0,2)]; }

int SpaceSystem::maxFuel(int t){
    // SPACE.EXE file 0x0974.
    if(t==0) return 1000;
    if(t==1) return 5000;
    if(t==2) return 2500;
    return 0;
}
int SpaceSystem::maxShield(int t){
    // SPACE.EXE file 0x09AB.
    if(t==0) return 1000;
    if(t==1) return 1000;
    if(t==2) return 5000;
    return 0;
}
const char* SpaceSystem::shipTypeName(int t){
    // The executable presents ports as left/right/top/bottom rather than naming
    // these three hull classes. These neutral labels avoid inventing lore names.
    static constexpr const char* N[3]={"우주 왕복선","1형 우주선","2형 우주선"};
    return (t>=0&&t<3)?N[t]:"우주선";
}

SpaceSector& SpaceSystem::sector(SpaceState& s) const { return s.sectors[static_cast<std::size_t>(sectorIndex(s.sectorX,s.sectorY))]; }
const SpaceSector& SpaceSystem::sector(const SpaceState& s) const { return s.sectors[static_cast<std::size_t>(sectorIndex(s.sectorX,s.sectorY))]; }
SpaceCraft* SpaceSystem::currentCraft(SpaceState& s) const {
    if(s.currentSlot<0||s.currentSlot>=4) return nullptr;
    auto& c=sector(s).craft[static_cast<std::size_t>(s.currentSlot)]; return c.active()?&c:nullptr;
}
const SpaceCraft* SpaceSystem::currentCraft(const SpaceState& s) const {
    if(s.currentSlot<0||s.currentSlot>=4) return nullptr;
    const auto& c=sector(s).craft[static_cast<std::size_t>(s.currentSlot)]; return c.active()?&c:nullptr;
}

void SpaceSystem::generateSector(SpaceSector& q,int,int){
    q=SpaceSector{};
    for(auto& c:q.craft)c.type=-1;

    // SPACE.EXE file 0x0BAF. The same 16-bit RNG used by OUT.EXE lives at
    // SPACE file 0x61FF, so DosRng is reused verbatim.
    q.enemies=rng_.range(0,240)/40;
    if(q.enemies>3) q.enemies=0;

    int px=0,py=0,bx=0,by=0;
    if(rng_.range(1,200)<77){ px=rng_.range(50,200); py=rng_.range(50,120); }
    q.starX=px; q.starY=py;

    if(rng_.range(1,200)<77){
        bx=rng_.range(50,200); by=rng_.range(50,100);
        // Original rejects the second landmark if either axis is less than 40
        // pixels away from the first one.
        if(std::abs(bx-px)<40 || std::abs(by-py)<40){ bx=0; by=0; }
    }
    q.baseX=bx; q.baseY=by;
    if(bx==0) return;

    const int countRoll=rng_.range(0,98)%3;
    if(countRoll==0) return;
    const int port0=rng_.range(0,199)%4;
    const int type0=rng_.range(0,98)%3;
    auto& a=q.craft[0];
    a.type=type0; a.fuel=maxFuel(type0); a.shield=maxShield(type0);
    a.x=portX(bx,port0,type0); a.y=portY(by,port0,type0); a.heading=oppositeHeading(port0);
    if(countRoll!=2) return;

    int type1;
    do type1=rng_.range(0,199)%3; while(type1==type0);
    int port1;
    do port1=rng_.range(0,98)%3; while(port1==port0);
    auto& b=q.craft[1];
    b.type=type1; b.fuel=maxFuel(type1); b.shield=maxShield(type1);
    b.x=portX(bx,port1,type1); b.y=portY(by,port1,type1); b.heading=oppositeHeading(port1);
}

void SpaceSystem::buildHomeSector(SpaceState& s,const PlayerSave& p){
    // SPACE.EXE file 0x0FDD..0x10B2 overrides sector (3,3) after procedural generation.
    auto& q=s.sectors[static_cast<std::size_t>(sectorIndex(3,3))];
    q.enemies=0; q.starX=140; q.starY=70; q.baseX=200; q.baseY=100;
    for(auto& c:q.craft)c=SpaceCraft{} , c.type=-1;

    auto& type2=q.craft[0]; type2.type=2; type2.fuel=maxFuel(2); type2.shield=maxShield(2);
    type2.x=portX(q.baseX,3,2); type2.y=portY(q.baseY,3,2); type2.heading=oppositeHeading(3);

    auto& type1=q.craft[1]; type1.type=1; type1.fuel=maxFuel(1); type1.shield=maxShield(1);
    type1.x=portX(q.baseX,0,1); type1.y=portY(q.baseY,0,1); type1.heading=oppositeHeading(0);

    auto& shuttle=q.craft[2]; shuttle.type=0; shuttle.fuel=p.shuttleStat0; shuttle.shield=p.shuttleStat1;
    shuttle.x=260; shuttle.y=90; shuttle.heading=0;
}

void SpaceSystem::generateUniverse(SpaceState& s,const PlayerSave& p){
    // SPACE.EXE 0x0F8E seeds its RNG with (PLAYER +0x32) * 3 / 2.
    rng_.seed(static_cast<std::uint16_t>((static_cast<std::uint32_t>(p.rngSeedWord)*3u)/2u));
    for(int x=0;x<7;++x) for(int y=0;y<7;++y)
        generateSector(s.sectors[static_cast<std::size_t>(sectorIndex(x,y))],x,y);
    buildHomeSector(s,p);
}

void SpaceSystem::spawnEnemyTarget(SpaceState& s){
    if(sector(s).enemies<=0){s.enemyX=s.enemyY=0;return;}
    // SPACE.EXE file 0x274B: initial target appears inside x=40..280, y=40..120.
    s.enemyX=rng_.range(40,255); // RNG caps max to 255; continue reduction as the executable does.
    if(s.enemyX<40) s.enemyX=40;
    // The executable asks for 280, but its RNG routine caps max at 255.
    s.enemyY=rng_.range(40,120);
    s.enemySprite=0;
    s.enemyAdvanceCounter=0;
}


int SpaceSystem::roundDosFixed(int v){
    // SPACE.EXE 0x11FD. 8086 IDIV truncates toward zero, as C++ integer / does.
    if(v%100>49) v+=100;
    if(v%1000>499) v+=1000;
    return v;
}

void SpaceSystem::initStar(SpaceState& s,int i){
    // SPACE.EXE 0x1D21: random quadrant + signed magnitudes around the reticle.
    const int quadrant=rng_.range(0,255)/64;
    int x=rng_.range(2,255)/2;
    int y=rng_.range(4,255)/4;
    if(quadrant==1||quadrant==2)x=-x;
    if(quadrant>1)y=-y;
    const int sx=s.crosshairX+x, sy=s.crosshairY+y;
    if(!(sx>20&&sx<300&&sy>20&&sy<140))x=y=5000;
    s.starX[static_cast<std::size_t>(i)]=x;
    s.starY[static_cast<std::size_t>(i)]=y;
}

void SpaceSystem::initStars(SpaceState& s){
    // SPACE.EXE 0x1FDA: forty vectors; previous point starts at reticle+3.
    for(int i=0;i<40;++i){
        initStar(s,i);
        s.starPrevX[static_cast<std::size_t>(i)]=s.crosshairX+3;
        s.starPrevY[static_cast<std::size_t>(i)]=s.crosshairY+3;
    }
    s.starsInitialized=true;
}

void SpaceSystem::updateStars(SpaceState& s){
    if(!s.starsInitialized)initStars(s);
    for(int i=0;i<40;++i){
        auto& rx=s.starX[static_cast<std::size_t>(i)];
        auto& ry=s.starY[static_cast<std::size_t>(i)];
        int dx=rx/8,dy=ry/6;
        if(dx==0)dx=signum(rx); if(dy==0)dy=signum(ry);
        rx+=dx; ry+=dy;
        int x=s.crosshairX+rx,y=s.crosshairY+ry;
        if(!(x>20&&x<300&&y>20&&y<140)){initStar(s,i);x=s.crosshairX+s.starX[i];y=s.crosshairY+s.starY[i];}
        if(s.starMode==1 && x>20&&x<300&&y>20&&y<140) s.starTrail.emplace_back(x,y);
        s.starPrevX[static_cast<std::size_t>(i)]=x; s.starPrevY[static_cast<std::size_t>(i)]=y;
    }
}

void SpaceSystem::enter(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    s=SpaceState{}; s.active=true; s.sectorX=3; s.sectorY=3; s.currentSlot=2;
    s.crosshairX=159; s.crosshairY=79;
    generateUniverse(s,p);
    spawnEnemyTarget(s);
    m.push_back("이륙합니다!");
    if(const auto* c=currentCraft(s); c && c->shield==0){
        // SPACE.EXE startup 0x3DE1..0x3DF6 is intentionally different from
        // in-flight shield depletion: if the initial Shuttle shield word is
        // already zero, it clears the saved Shuttle resources and returns to
        // OUT.EXE's death path without printing "Thy shield is drained!".
        p.shuttleStat0=0;
        p.shuttleStat1=0;
        p.hits=0;
        p.food=0;
        s.active=false;
    }
}

bool SpaceSystem::isAtDock(const SpaceState& s,int slot) const{
    if(slot<0||slot>=4) return false;
    const auto& q=sector(s); const auto& c=q.craft[static_cast<std::size_t>(slot)];
    if(!c.active()||q.baseX==0) return false;
    for(int port=0;port<4;++port)
        if(c.x==portX(q.baseX,port,c.type)&&c.y==portY(q.baseY,port,c.type)&&c.heading==oppositeHeading(port)) return true;
    return false;
}

void SpaceSystem::direction(SpaceState& s,PlayerSave& p,int code,std::vector<std::string>& m){
    s.sectorScan=false;
    auto* c=currentCraft(s); if(!c) return;
    code=std::clamp(code,1,4);
    if(s.frontView){
        static constexpr const char* MSG[5]={"","조준점 좌측으로","조준점 우측으로","조준점 상방으로","조준점 하방으로"};
        m.push_back(MSG[code]);
        if(c->fuel<2){m.push_back("연료가 부족합니다!");return;}
        c->fuel-=2;
        // SPACE.EXE file 0x1B82..0x1BB2 does not move the reticle immediately.
        // It installs a +/-4 aim velocity; the continuous front-view loop at
        // file 0x1F74 advances the reticle and stops it at the screen limits.
        if(code==1){ s.aimVelocityX=-4; s.aimVelocityY=0; }
        else if(code==2){ s.aimVelocityX=4; s.aimVelocityY=0; }
        else if(code==3){ s.aimVelocityX=0; s.aimVelocityY=-4; }
        else { s.aimVelocityX=0; s.aimVelocityY=4; }
        return;
    }

    static constexpr const char* MSG[5]={"","반시계 방향 회전","시계 방향 회전","추진","역추진"};
    m.push_back(MSG[code]);
    const int cost=(code<=2)?2:5;
    if(c->fuel==0){m.push_back("연료가 없습니다!!");m.push_back("영원히 떠다닐 건가요?!?");return;}
    const int actual=std::min(cost,c->fuel);

    // SPACE.EXE 0x1933..0x1972: while the craft is geometrically seated in a
    // dock, every top-view direction command except Retro rams the base.  The
    // failed command does not spend its pending fuel cost; it produces Crunch
    // damage instead.  Retro is the sole normal undocking manoeuvre.
    if(isAtDock(s,s.currentSlot) && code!=4){
        applyCrunchDamage(s,p,m,false);
        return;
    }

    c->fuel-=actual;
    if(code==4 && s.docked){ s.docked=false; s.choosingShip=false; }

    if(code==1 || code==2){
        // SPACE.EXE DS:0BD2 table, indexed by command and current heading.
        static constexpr int TURN[2][4]={{3,2,0,1},{2,3,1,0}};
        c->heading=TURN[code-1][std::clamp(c->heading,0,3)]; s.thrustCounter=0; return;
    }
    const int dv=(code==3)?1:-1;
    if(c->heading==0) s.velocityX-=dv;
    else if(c->heading==1) s.velocityX+=dv;
    else if(c->heading==2) s.velocityY-=dv;
    else s.velocityY+=dv;
    s.velocityX=clampVelocity(s.velocityX); s.velocityY=clampVelocity(s.velocityY);
    if(code==3) s.thrustCounter+=10; else s.thrustCounter=0;
}

void SpaceSystem::toggleView(SpaceState& s,PlayerSave&,std::vector<std::string>& m){
    s.sectorScan=false;
    m.push_back("시점 바꾸기");
    auto* c=currentCraft(s); if(!c)return;
    if(!s.frontView){
        if(isAtDock(s,s.currentSlot)){m.push_back("도킹 상태에서는 시점을 바꿀 수 없습니다.");return;}
        if(c->fuel==0){m.push_back("연료가 부족합니다!");return;}
        s.frontView=true; s.aimVelocityX=s.aimVelocityY=0;
        // SPACE.EXE 0x131F..0x13B6: front speed is |top velocity|, clamped 1..8.
        int speed=static_cast<int>(std::sqrt(static_cast<double>(s.velocityX*s.velocityX+s.velocityY*s.velocityY)));
        speed=std::clamp(speed,1,8);
        if(s.velocityX==0&&s.velocityY==0){
            if(c->heading==0)s.velocityX=-1; else if(c->heading==1)s.velocityX=1;
            else if(c->heading==2)s.velocityY=-1; else s.velocityY=1;
            speed=1;
        }
        s.frontSpeed=s.frontEntrySpeed=speed; s.starMode=2; s.starTrail.clear(); initStars(s);
        return;
    }
    if(sector(s).enemies!=0){m.push_back("모든 적기를 격추시키는 것이 먼저입니다!");return;}
    // SPACE.EXE 0x1231/0x1272: rescale top-view inertia by speed ratio using
    // the executable's two-stage fixed-point rounding helper at 0x11FD.
    const int denom=std::max(1,s.frontEntrySpeed);
    const int factor=(s.frontSpeed*1000)/denom;
    auto scale=[&](int v){int q=roundDosFixed(v*factor)/1000;if(q>6)q=6;return q;};
    s.velocityX=scale(s.velocityX); s.velocityY=scale(s.velocityY);
    if(s.velocityX==0&&s.velocityY==0){
        if(c->heading==0)s.velocityX=-1; else if(c->heading==1)s.velocityX=1;
        else if(c->heading==2)s.velocityY=-1; else s.velocityY=1;
    }
    s.frontView=false; s.aimVelocityX=s.aimVelocityY=0; s.starTrail.clear();
}

void SpaceSystem::setSpeed(SpaceState& s,PlayerSave&,int speed,std::vector<std::string>& m){
    s.sectorScan=false; if(!s.frontView||s.hyperPhase!=0)return;
    speed=std::clamp(speed,1,8); m.push_back("속도 "+std::to_string(speed));
    auto*c=currentCraft(s);if(!c)return; const int cost=std::abs(speed-s.frontSpeed)*4;
    if(c->fuel<cost){m.push_back("연료가 부족합니다!");return;}
    c->fuel-=cost; s.frontSpeed=speed;
}

void SpaceSystem::scan(SpaceState& s,std::vector<std::string>&){
    s.sectorScan=true;
    s.sectorScanStartMs=SDL_GetTicks();
}
void SpaceSystem::dismissScan(SpaceState& s){
    s.sectorScan=false;
    s.sectorScanStartMs=0;
}

bool SpaceSystem::transferCraft(SpaceState& s,int nx,int ny,bool hyperJumpPlacement){
    auto old=currentCraft(s)?*currentCraft(s):SpaceCraft{};
    const int oldSectorX=s.sectorX, oldSectorY=s.sectorY, oldSlot=s.currentSlot;
    const int tx=wrap7(nx), ty=wrap7(ny);
    auto& target=s.sectors[static_cast<std::size_t>(sectorIndex(tx,ty))];
    int slot=-1;
    for(int i=0;i<4;++i) if(!target.craft[static_cast<std::size_t>(i)].active()){slot=i;break;}
    // SPACE.EXE file 0x20EC..0x2131 treats a full four-record sector as an
    // abnormal/fatal condition rather than overwriting an existing craft.
    if(slot<0) return false;
    auto& oldSector=s.sectors[static_cast<std::size_t>(sectorIndex(oldSectorX,oldSectorY))];
    if(oldSlot>=0&&oldSlot<4) oldSector.craft[static_cast<std::size_t>(oldSlot)].type=-1;
    s.sectorX=tx; s.sectorY=ty; s.currentSlot=slot;
    target.craft[static_cast<std::size_t>(slot)]=old;
    if(auto* c=currentCraft(s)){
        // HyperJump's sector-transfer routine writes the fixed arrival point
        // (260,90) after copying the twelve-byte craft record.
        if(hyperJumpPlacement){ c->x=260; c->y=90; }
    }
    spawnEnemyTarget(s);
    return true;
}

void SpaceSystem::hyperJump(SpaceState& s,PlayerSave&,std::vector<std::string>& m){
    s.sectorScan=false;
    auto*c=currentCraft(s);if(!c)return;
    // SPACE.EXE prints the command name first, then appends either '?' on an
    // invalid-view path or '...' immediately before the blocking jump sequence.
    // The SDL log receives the already-composed command row so the Korean
    // presentation can preserve the original single-row overwrite behavior.
    if(!s.frontView){m.push_back("하이퍼점프");m.push_back("하이퍼점프는 정면 시점에서만 가능합니다.");return;}
    if(s.hyperPhase!=0)return;
    if(c->fuel<100){m.push_back("하이퍼점프");m.push_back("연료가 부족합니다!");return;}
    m.push_back("하이퍼점프...");
    // SPACE.EXE 0x2258..0x239F: the command is a blocking multi-phase animation.
    // SDL stores those phases incrementally; M252 paces phase 1/4 by the original 70-Hz retrace waits.
    s.aimVelocityX=s.aimVelocityY=0;
    s.hyperOriginalSpeed=std::clamp(s.frontSpeed,0,8);
    s.hyperDelay=(s.hyperOriginalSpeed<4)?10:15;
    s.hyperSpeedCursor=s.hyperOriginalSpeed;
    s.hyperTick=0; s.appleHyperVisualStage=0; s.appleHyperVisualCalls=0; s.hyperWaitRetraces=0; s.hyperPhase=1; s.starMode=2; s.starTrail.clear();
}

// HF63: The Apple II CPU's actual $9811 sound command is authoritative.
// One packed atomic snapshot describes its stage and completed $9497 calls.
// Visual updates catch up with those source calls, never an independent DOS
// retrace timer. The original 1986 audio and visual stop together.
void SpaceSystem::updateAppleHyperProgress(
    SpaceState& s,PlayerSave& p,std::vector<std::string>&m,std::uint64_t progress){
    if(!s.active || s.hyperPhase==0 || progress==0)return;
    const int stage=static_cast<int>((progress>>56)&0xff);
    const int speed=static_cast<int>((progress>>48)&0xff);
    const int calls=static_cast<int>(progress&0xffffffffu);
    // A previous jump may have finished between the new H-key command
    // and the first subsequent audio callback. Never complete a NEW jump
    // based on the previous jump's stale stage-5 snapshot.
    if(stage==5 && s.appleHyperVisualStage==0)return;
    if(stage==5){
        s.hyperPhase=4;
        s.hyperSpeedCursor=s.hyperOriginalSpeed;
        updateHyperJump(s,p,m); // existing fuel/sector-transfer completion logic
        return;
    }
    if(stage<1 || stage>4)return;
    if(stage!=s.appleHyperVisualStage){
        s.appleHyperVisualStage=stage;
        s.appleHyperVisualCalls=0;
        s.hyperPhase=stage;
        s.hyperTick=0;
        if(stage==2){
            // Apple II 1986 SPA $9842/$9844: deduct 100 fuel immediately
            // before the lightspeed burst, after the acceleration phase.
            if(auto* craft=currentCraft(s))
                craft->fuel=std::max(0,craft->fuel-100);
            s.starTrail.clear();initStars(s);s.starMode=1;
            m.push_back("광속 추진!");
        }else if(stage==3){
            s.starTrail.clear();initStars(s);s.starMode=3;
        }else{
            s.starMode=2;
        }
    }
    s.frontSpeed=std::clamp(speed,1,8);
    const int delta=std::max(0,calls-s.appleHyperVisualCalls);
    // At ~60fps, $9868/$9886 generate roughly 1-2 star frames per host
    // frame. Cap recovery after OS stalls rather than freezing the UI.
    for(int i=0;i<std::min(delta,20);++i)updateStars(s);
    s.appleHyperVisualCalls=calls;
}

void SpaceSystem::updateHyperJump(SpaceState&s,PlayerSave&p,std::vector<std::string>&m){
    auto*c=currentCraft(s);if(!c){s.hyperPhase=0;return;}
    if(s.hyperPhase==1){
        // SPACE.EXE 0x22A2..0x22C5 calls 0x3530 delay+1 times at each
        // speed. 0x3530 performs the front update and then waits the speed's
        // DS:0C9E retrace count (speed 1..8 => 8..1; speed 0 => no wait).
        s.frontSpeed=std::clamp(s.hyperSpeedCursor,0,8);
        // HF56: one extra 70-Hz retrace on each acceleration update.
        // Delays arrival at lightspeed without changing burst duration.
        s.hyperWaitRetraces=(s.frontSpeed==0)?0:(10-s.frontSpeed);
        updateStars(s);
        if(++s.hyperTick>s.hyperDelay){s.hyperTick=0;++s.hyperSpeedCursor;}
        if(s.hyperSpeedCursor>=9){
            if(auto* craft=currentCraft(s))
                craft->fuel=std::max(0,craft->fuel-100);
            s.frontSpeed=0;s.starTrail.clear();initStars(s);s.starMode=1;s.hyperTick=0;s.hyperPhase=2;m.push_back("광속 추진!");
        }
        return;
    }
    if(s.hyperPhase==2){
        s.hyperWaitRetraces=0;
        updateStars(s); // 201 calls, mode 1 leaves old points: radial streaks.
        if(++s.hyperTick>=141){s.starTrail.clear();initStars(s);s.starMode=3;s.hyperTick=0;s.hyperPhase=3;}
        return;
    }
    if(s.hyperPhase==3){
        s.hyperWaitRetraces=0;
        updateStars(s); // 200 calls, erase/move phase.
        if(++s.hyperTick>=140){s.starMode=2;s.hyperSpeedCursor=8;s.hyperTick=0;s.hyperPhase=4;}
        return;
    }
    if(s.hyperPhase==4){
        if(s.hyperSpeedCursor>s.hyperOriginalSpeed){
            s.frontSpeed=s.hyperSpeedCursor;
            // HF55: add one 70-Hz retrace per deceleration frame.
            // At common entry speeds this adds approximately two seconds
            // across the full recovery, without a sudden end-of-jump pause.
            s.hyperWaitRetraces=11-s.frontSpeed;
            updateStars(s);
            if(++s.hyperTick>s.hyperDelay){s.hyperTick=0;--s.hyperSpeedCursor;}
            return;
        }
        // Fuel was already deducted on entering lightspeed (Apple SPA $9842).
        s.frontSpeed=s.hyperOriginalSpeed;
        int dx=signum(s.velocityX),dy=signum(s.velocityY);
        if(dx==0&&dy==0){if(c->heading==0)dx=-1;else if(c->heading==1)dx=1;else if(c->heading==2)dy=-1;else dy=1;}
        if(!transferCraft(s,s.sectorX+dx,s.sectorY+dy,true)){m.push_back("하이퍼점프 목적지가 가득 찼습니다.");}
        s.enemyAdvanceCounter=0;s.starMode=2;s.hyperPhase=0;s.hyperTick=0;s.hyperWaitRetraces=0;s.starTrail.clear();initStars(s);m.push_back("하이퍼점프 완료");
        return;
    }
}

void SpaceSystem::fire(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    s.sectorScan=false;
    if(!s.frontView){m.push_back("발포하기?");return;}
    m.push_back("발포!");
    auto* c=currentCraft(s); if(!c)return;
    if(c->fuel<10){m.push_back("연료가 부족합니다!");return;}
    c->fuel-=10;

    // SPACE.EXE 0x24B7..0x24E4: every successful Fire command draws two
    // light-red laser lines from the lower-left/lower-right flight-window
    // Apple-derived inset emitters (40,127)/(280,127) to the gunsight; shot sound 8,
    // then erases the same two lines and redraws the gunsight.  SDL keeps the
    // exact geometry visible briefly so the DOS draw/erase pulse is observable.
    s.playerShotX=s.crosshairX;
    s.playerShotY=s.crosshairY;
    s.playerShotUntilMs=SDL_GetTicks()+80u;

    auto& q=sector(s);
    if(q.enemies<=0 || s.enemyX==0){return;}
    // SPACE.EXE file 0x24F2..0x254C: hitbox is asymmetric and depends on
    // the seven front-view enemy growth stages.  Stage 0 is intentionally
    // unhittable because Y-min(5) > Y-max(4).
    const int stage=std::clamp(s.enemySprite,0,6);
    const int dx=s.crosshairX-s.enemyX, dy=s.crosshairY-s.enemyY;
    if(dx>=ENEMY_X_MIN[stage] && dx<=ENEMY_X_MAX[stage] &&
       dy>=ENEMY_Y_MIN[stage] && dy<=ENEMY_Y_MAX[stage]){
        s.playerHitSfxPending=true;
        m.push_back("적중!!!");
        p.experience=static_cast<std::uint16_t>(
            std::min(9999,static_cast<int>(p.experience)+100));
        ++p.spaceKills;
        --q.enemies;
        if(p.spaceKills==20){m.push_back("당신은 우주 영웅에 등극하였습니다!");}
        else if(p.spaceKills>20)m.push_back("당신은 이미 우주 영웅입니다!");
        // Keep the struck enemy rendered in place until all eight blinks end.
        s.killBlinkStartMs=SDL_GetTicks();
        s.killBlinkUntilMs=s.killBlinkStartMs+512u;
        s.playerShotUntilMs=0;
    }
}

void SpaceSystem::centerOrPass(SpaceState& s,std::vector<std::string>& m){
    s.sectorScan=false;
    if(!s.frontView){m.push_back("통과");return;}
    m.push_back("중심");
    s.crosshairX=159;s.crosshairY=79;
    s.aimVelocityX=s.aimVelocityY=0;
}
void SpaceSystem::invalidCommand(const char* t,std::vector<std::string>& m) const { m.push_back(t); }

bool SpaceSystem::chooseShip(SpaceState& s,PlayerSave&,int oneBased,std::vector<std::string>& m){
    if(!s.choosingShip)return false;
    const int port=oneBased-1;
    if(port<0||port>=4){m.push_back("그 부두는 사용할 수 없습니다.");m.push_back("우주선을 선택하세요:");return true;}
    int slot=-1;
    const auto& q=sector(s);
    for(int i=0;i<4;++i){
        const auto& c=q.craft[static_cast<std::size_t>(i)];
        if(c.active() && c.x==portX(q.baseX,port,c.type) && c.y==portY(q.baseY,port,c.type)) {slot=i;break;}
    }
    if(slot<0){m.push_back("그 부두는 사용할 수 없습니다.");m.push_back("우주선을 선택하세요:");return true;}
    s.currentSlot=slot; s.choosingShip=false; s.docked=true;
    static constexpr const char* PORT[4]={"좌측","우측","상단","중앙"};
    m.push_back(std::string("당신은 ")+PORT[port]+" 우주선에 있습니다.");
    return true;
}

void SpaceSystem::killPlayerInSpace(SpaceState& s,PlayerSave& p,std::vector<std::string>& m,
                                    bool shieldDrainedMessage){
    p.hits=0; p.shuttleStat0=0; p.shuttleStat1=0; s.active=false;
    if(shieldDrainedMessage) m.push_back("방어막이 모두 소진되었습니다!");
}

void SpaceSystem::applyCrunchDamage(SpaceState& s,PlayerSave& p,std::vector<std::string>& m,
                                    bool reverseVelocity){
    auto* c=currentCraft(s); if(!c)return;
    // SPACE.EXE 0x370A..0x375E and helper 0x0B10: Crunch damage is exactly
    // floor(currentShield/4)+5, clamped at zero.  A surviving moving collision
    // reverses both top-view velocity axes.  The dock-command collision path
    // uses the same damage helper but has no velocity reversal of its own.
    m.push_back("쿵!");
    s.crunchSfxPending=true;
    const int damage=c->shield/4+5;
    c->shield=std::max(0,c->shield-damage);
    if(c->shield==0){
        killPlayerInSpace(s,p,m,true);
        return;
    }
    if(reverseVelocity){
        s.velocityX=-s.velocityX;
        s.velocityY=-s.velocityY;
    }
}

void SpaceSystem::checkLandingOrDock(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    auto* c=currentCraft(s); if(!c)return;
    const auto& q=sector(s);

    // SPACE.EXE 0x2865: entering a 25x25 box around the sector's star is fatal.
    if(q.starX!=0){
        // Apple II 1986 SPA $8B67..$8B85 calls $9CE2 for object 9 vs 8:
        // squared coordinate distance followed by integer square root
        // (successive odd-number subtraction), then compares A with $15.
        const int dx=c->x-q.starX,dy=c->y-q.starY;
        const int squared=dx*dx+dy*dy;
        int remaining=squared, odd=1, integerDistance=0;
        while(remaining>=odd){remaining-=odd;odd+=2;++integerDistance;}
        if(integerDistance<0x15){
        m.push_back("우주선이 항성 주변에서 녹습니다!");
        killPlayerInSpace(s,p,m); return;
        }
    }

    // SPACE.EXE 0x2931: the Ultima world itself is a fixed landing target in
    // home sector (3,3), centered at local coordinates (60,75), not at the
    // sector star coordinate.
    if(s.sectorX==3&&s.sectorY==3&&std::abs(c->x-60)<25&&std::abs(c->y-75)<25){
        if(c->type==0){
            m.push_back("안전하게 착륙했습니다!");
            p.shuttleStat0=static_cast<std::uint16_t>(std::max(0,c->fuel));
            p.shuttleStat1=static_cast<std::uint16_t>(std::max(0,c->shield));
            s.active=false; return;
        }
        m.push_back("오직 열 방어막을 갖춘 우주 왕복선만이 착륙 가능합니다!");
        killPlayerInSpace(s,p,m);return;
    }

    // SPACE.EXE 0x3941 -> 0x3767: geometric dock contact is sufficient.
    // The service itself stops the ship; incoming velocity is NOT required to be zero.
    if(isAtDock(s,s.currentSlot)){
        if(!s.docked){
            s.docked=true; s.velocityX=0; s.velocityY=0; s.thrustCounter=0;
            m.push_back("도킹 완료! 기지에 잘 오셨습니다!");
            // SPACE.EXE 0x37DF checks ready armour before the 500p fee. Armour
            // indices 0..3 do not survive the vacuum when the space doors open.
            if(p.readyArmour<4){
                m.push_back("우주선 문이 열리자 당신은 완전한 진공 상태에 노출됩니다!");
                killPlayerInSpace(s,p,m); return;
            }
            if(p.coin<500){
                m.push_back("도킹 비용으로 500 코인을 지불해야만 연료를 채워 드립니다."); return;
            }
            p.coin=static_cast<std::uint16_t>(p.coin-500);
            c->fuel=maxFuel(c->type);c->shield=maxShield(c->type);
            int available=0;for(const auto& z:q.craft)if(z.active())++available;
            if(available>1){m.push_back("우주선을 선택하세요:");s.choosingShip=true;}
        }
    } else s.docked=false;
}

void SpaceSystem::applyTopMotionTick(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    if(s.frontView||s.docked)return;
    auto* c=currentCraft(s); if(!c)return;

    // SPACE.EXE 0x3309/0x3329: the thrust-exhaust lifetime is counted in
    // top-view movement ticks, not render frames.  Thrust adds 10; each tick
    // consumes one before the ship is redrawn at its new position.
    if(s.thrustCounter>0)--s.thrustCounter;

    // SPACE.EXE file 0x36A8 / 0x36D9: top-view position wraps inside the
    // logical flight window, not into an adjacent sector. Sector changes are
    // a HyperJump operation.
    int nx=c->x+s.velocityX; if(nx>275)nx=20; else if(nx<20)nx=275;
    int ny=c->y+s.velocityY; if(ny>130)ny=10; else if(ny<10)ny=130;
    c->x=nx;c->y=ny;
    if(topCollision(s,s.currentSlot,nx,ny)){
        // Star/home-world/dock contacts are resolved before generic Crunch,
        // exactly as SPACE.EXE 0x370A first calls the two landing/fatal tests
        // and the movement path checks dock geometry on collision-free contact.
        checkLandingOrDock(s,p,m);
        if(!s.active||s.docked)return;
        s.collisionX=nx;s.collisionY=ny;s.collisionFlashFrames=1;
        applyCrunchDamage(s,p,m,true);
        return;
    }
    checkLandingOrDock(s,p,m);
}

void SpaceSystem::maybeAlienFire(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    auto*c=currentCraft(s);if(!c||!s.frontView||s.hyperPhase!=0)return;
    if(s.enemySprite==0||s.enemyX==0)return;
    // SPACE.EXE 0x33D7: 1..3 out of 1..200 enters the shot path.
    if(rng_.range(1,200)>=4)return;
    m.push_back("적기가 발포합니다!");
    const int roll=rng_.range(1,255);
    if(roll<64){s.alienShotX1=159;s.alienShotY1=135;}
    else if(roll<128){s.alienShotX1=159;s.alienShotY1=20;}
    else if(roll<=192){s.alienShotX1=290;s.alienShotY1=79;}
    else{s.alienShotX1=20;s.alienShotY1=79;}
    const int st=std::clamp(s.enemySprite,0,6);
    s.alienShotX0=s.enemyX+ENEMY_X_MAX[st]/2;
    s.alienShotY0=s.enemyY+ENEMY_Y_MAX[st];
    // DOS draws the red beam, delays, then draws it black. A short SDL lifetime
    // preserves the exact geometry without pretending frame timing is cycle exact.
    s.alienShotFrames=3;
    s.alienFireSfxPending=true;
    if(roll>=64)return;
    s.alienHitSfxPending=true;
    m.push_back("적중했습니다!");
    c->shield=std::max(0,c->shield-std::min(c->shield,321));
    if(c->shield==0){killPlayerInSpace(s,p,m,true);}
}

void SpaceSystem::applyFrontTick(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    if(!s.frontView)return;
    updateStars(s);

    // SPACE.EXE file 0x1F74..0x1FBD: aim commands install velocity, then the
    // continuous loop advances until the next step would hit an edge.
    if(s.aimVelocityX!=0){
        const int old=s.crosshairX;
        s.crosshairX+=s.aimVelocityX;
        if(s.crosshairX>=300||s.crosshairX<=20){s.crosshairX=old;s.aimVelocityX=0;}
    }
    if(s.aimVelocityY!=0){
        const int old=s.crosshairY;
        s.crosshairY+=s.aimVelocityY;
        if(s.crosshairY>=140||s.crosshairY<=20){s.crosshairY=old;s.aimVelocityY=0;}
    }

    auto& q=sector(s);
    if(q.enemies<=0){s.enemyX=s.enemyY=0;return;}
    if(s.enemyX==0){spawnEnemyTarget(s);return;}

    // SPACE.EXE file 0x3578..0x35A9: enemy target moves one logical pixel
    // away from the player's reticle on each axis every flight-loop tick.
    s.enemyX-=signum(s.crosshairX-s.enemyX);
    s.enemyY-=signum(s.crosshairY-s.enemyY);
    if(s.enemyX>280||s.enemyX<30||s.enemyY>125||s.enemyY<24){
        spawnEnemyTarget(s); return;
    }

    // Normal combat (not the internal HyperJump animation) advances the seven
    // enemy growth frames every 16 loop ticks.  The executable respawns once
    // stage 6 is exceeded.
    if(++s.enemyAdvanceCounter>15){
        s.enemyAdvanceCounter=0;
        ++s.enemySprite;
        if(s.enemySprite>6){spawnEnemyTarget(s);return;}
    }
    maybeAlienFire(s,p,m);
}

void SpaceSystem::updateFrame(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    if(!s.active)return;
    if(s.killBlinkUntilMs!=0){
        if(static_cast<std::int32_t>(SDL_GetTicks()-s.killBlinkUntilMs)<0)
            return; // Pause stars, flight, alien fire, and target growth.
        s.killBlinkUntilMs=0;
        s.killBlinkStartMs=0;
        if(sector(s).enemies>0)spawnEnemyTarget(s);
        else s.enemyX=s.enemyY=0;
        return; // Resume motion on the next ordinary game tick.
    }
    if(s.alienShotFrames>0)--s.alienShotFrames;
    if(s.collisionFlashFrames>0)--s.collisionFlashFrames;
    if(s.hyperPhase!=0) updateHyperJump(s,p,m);
    else if(s.frontView) applyFrontTick(s,p,m);
    else applyTopMotionTick(s,p,m);
    if(auto*c=currentCraft(s);c&&c->shield<=0&&s.active)killPlayerInSpace(s,p,m,true);
}

void SpaceSystem::updateAfterCommand(SpaceState& s,PlayerSave& p,std::vector<std::string>& m){
    if(!s.active)return;
    // SPACE.EXE updates flight continuously while it polls the keyboard; a
    // completed command itself does not add an extra inertial movement step.
    if(auto* c=currentCraft(s);c&&c->shield<=0)killPlayerInSpace(s,p,m,true);
}

void SpaceSystem::draw(SDL_Renderer* r,const PixelFont& font,const SpaceState& s,const PlayerSave& p,SDL_Color white) const{
    // SPACE.EXE draws its gray flight bezel inside the persistent OUT.EXE-style
    // blue/white gameplay frame. main.cpp overlays that shared frame after the
    // SPACE scene, exactly as the original screen capture shows.
    drawSpaceFrame(r);
    const auto*c=currentCraft(s);
    if(s.sectorScan){
        drawSectorScan(r,s);
        // 0x16EB selects DS:5ADA (EGA 11, light cyan), then 0x1701 prints
        // Original "Sector Scan:" title at BIOS text column 5,row4; localized below.
        font.draw(r,80,64,"구역 검색:",CYAN);
    }else if(s.frontView) drawFightExact(r,s,white);
    else drawTopExact(r,s);

    // SPACE.EXE uses the same four-row lower-right status pane geometry as the
    // outdoor UI. Localized labels keep equal visual width so the values align.
    if(c){
        constexpr int labelX=31*16, valueX=36*16+3*8;
        const SDL_Color sh=(c->shield<100)?EGA[12]:EGA[11];
        const SDL_Color fu=(c->fuel<100)?EGA[12]:EGA[11];
        const int numericRightX=valueX+4*8;
        auto drawNumber=[&](int row,std::uint16_t value,SDL_Color color){
            const auto digits=std::to_string(std::min<unsigned>(value,9999u));
            font.draw(r,numericRightX-font.measure(digits),row*16,digits,color);
        };
        font.draw(r,labelX,21*16,"방어막:",EGA[11]);
        drawNumber(21,static_cast<std::uint16_t>(std::max(0,c->shield)),sh);
        font.draw(r,labelX,22*16,"연  료:",EGA[11]);
        drawNumber(22,static_cast<std::uint16_t>(std::max(0,c->fuel)),fu);
        font.draw(r,labelX,23*16,"경험치:",EGA[11]);
        drawNumber(23,p.experience,EGA[11]);
        font.draw(r,labelX,24*16,"코  인:",EGA[11]);
        drawNumber(24,p.coin,EGA[11]);
    }
}
