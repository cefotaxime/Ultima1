#include <SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "OriginalData.h"
#include "PixelFont.h"

namespace {

constexpr int WIN_W=1200;
constexpr int WIN_H=800;
constexpr std::uint32_t DOS_TICK_NUM=65536u;
constexpr std::uint32_t DOS_TICK_DEN=1193182u;

constexpr std::uint32_t dosTicksToMs(std::uint32_t ticks){
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(ticks)*DOS_TICK_NUM*1000u + DOS_TICK_DEN-1u)/DOS_TICK_DEN);
}
constexpr std::uint32_t ANIM_MS=dosTicksToMs(3);

constexpr SDL_Color BLACK{0,0,0,255};
constexpr SDL_Color WHITE{255,255,255,255};
constexpr SDL_Color CYAN{85,255,255,255};
constexpr SDL_Color BLUE{0,0,170,255};
constexpr SDL_Color GREY{85,85,85,255};
constexpr SDL_Color YELLOW{255,255,85,255};
constexpr SDL_Color RED{255,85,85,255};
constexpr SDL_Color GREEN{85,255,85,255};

enum class Rule {
    WaterRotate,
    CastleHold,
    TownPulse,
    ToggleParity,
    RandomPair
};

enum class TileUse {
    PeriodicBitmap,
    PeriodicPair,
    StaticUsed,
    TransientFx,
    NoNormalDraw
};

struct AnimGroup {
    const char* name;
    int baseTile;
    Rule rule;
};

constexpr std::array<AnimGroup,19> GROUPS{{
    {"WATER",0,Rule::WaterRotate},
    {"CASTLE",4,Rule::CastleHold},
    {"TOWN",7,Rule::TownPulse},
    {"FRIGATE",14,Rule::ToggleParity},
    {"NESS CREATURE",19,Rule::RandomPair},
    {"GIANT SQUID",21,Rule::RandomPair},
    {"DRAGON TURTLE",23,Rule::RandomPair},
    {"PIRATE SHIP",25,Rule::ToggleParity},
    {"HOOD",27,Rule::RandomPair},
    {"BEAR",29,Rule::RandomPair},
    {"HIDDEN ARCHER",31,Rule::RandomPair},
    {"DARK KNIGHT",33,Rule::RandomPair},
    {"EVIL TRENT",35,Rule::RandomPair},
    {"THIEF",37,Rule::RandomPair},
    {"ORC",39,Rule::RandomPair},
    {"KNIGHT",41,Rule::RandomPair},
    {"NECROMANCER",43,Rule::RandomPair},
    {"EVIL RANGER",45,Rule::RandomPair},
    {"WANDERING WARLOCK",47,Rule::RandomPair},
}};

constexpr std::array<int,20> EXPECTED_WORLD_BLITTER_CALLS{{
    0x0F92,0x0FDA,0x1060,0x1122,0x11AD,0x1346,0x1A87,0x1AC6,
    0x8428,0x84A8,0x84F0,0x856C,0x85BE,0x8665,
    0xB1DB,0xB1FC,0xB21D,0xB247,0xB27E,0xC5B6
}};

std::vector<std::uint8_t> readAll(const std::string& path){
    std::ifstream f(path,std::ios::binary);
    if(!f) return {};
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}

bool hasBytes(const std::vector<std::uint8_t>& b,std::size_t begin,std::size_t end,
              std::initializer_list<std::uint8_t> needle){
    if(begin>end || end>b.size() || needle.size()>end-begin) return false;
    return std::search(b.begin()+static_cast<std::ptrdiff_t>(begin),
                       b.begin()+static_cast<std::ptrdiff_t>(end),
                       needle.begin(),needle.end()) !=
           b.begin()+static_cast<std::ptrdiff_t>(end);
}

std::vector<int> rel16CallSites(const std::vector<std::uint8_t>& b,int target){
    std::vector<int> out;
    const std::size_t lim=std::min<std::size_t>(b.size(),0x10000);
    for(std::size_t i=0;i+2<lim;++i){
        if(b[i]!=0xE8) continue;
        const std::int16_t rel=static_cast<std::int16_t>(
            static_cast<std::uint16_t>(b[i+1]) |
            (static_cast<std::uint16_t>(b[i+2])<<8));
        const int dest=(static_cast<int>(i)+3+static_cast<int>(rel))&0xFFFF;
        if(dest==target) out.push_back(static_cast<int>(i));
    }
    return out;
}

bool verifyAnimationOracle(const std::string& data,std::string& error){
    const auto out=readAll(data+"/OUT.EXE");
    if(out.size()!=69858){ error="OUT.EXE must be 69,858 bytes"; return false; }

    // Exhaustive world-tile blitter inventory.  A7B4 is the standard EGATILES
    // 16x16 world-tile entry point; the original executable has exactly these
    // twenty call sites.
    const auto blits=rel16CallSites(out,0xA7B4);
    if(blits.size()!=EXPECTED_WORLD_BLITTER_CALLS.size() ||
       !std::equal(blits.begin(),blits.end(),EXPECTED_WORLD_BLITTER_CALLS.begin())){
        error="OUT.EXE world-tile blitter call-site inventory mismatch"; return false;
    }

    // A7CF is the low-level EGATILES blitter and is reached only through A7B4.
    const auto low=rel16CallSites(out,0xA7CF);
    if(low.size()!=1 || low[0]!=0xA7C4){
        error="OUT.EXE low-level EGATILES blitter inventory mismatch"; return false;
    }

    // OUT.EXE 0xA754: mutate only tile 0 (water) by rotating one EGA scanline.
    const auto waterCalls=rel16CallSites(out,0xA754);
    if(waterCalls!=std::vector<int>({0x925B,0xB19B}) ||
       !hasBytes(out,0xA754,0xA78E,{0x83,0xC6,0x78,0xB9,0x04,0x00,0xF3,0xA5}) ||
       !hasBytes(out,0xA754,0xA78E,{0xFD,0xBE,0x52,0x71,0xBF,0x5A,0x71,0xB9,0x3C,0x00,0xF3,0xA5})){
        error="OUT.EXE water-animation oracle mismatch"; return false;
    }

    // B157 is the only periodic overworld tile-animation dispatcher.
    const auto dispatchCalls=rel16CallSites(out,0xB157);
    if(dispatchCalls!=std::vector<int>({0x90DB})){
        error="OUT.EXE periodic animation dispatcher inventory mismatch"; return false;
    }
    if(!hasBytes(out,0xB157,0xB294,{0xF7,0x46,0x12,0x03,0x00}) ||
       !hasBytes(out,0xB157,0xB294,{0x82,0x7E,0x0C,0x07}) ||
       !hasBytes(out,0xB157,0xB294,{0x82,0x7E,0x0C,0x04}) ||
       !hasBytes(out,0xB157,0xB294,{0x3D,0x0E,0x00}) ||
       !hasBytes(out,0xB157,0xB294,{0x3D,0x19,0x00}) ||
       !hasBytes(out,0xB157,0xB294,{0x3D,0x13,0x00}) ||
       !hasBytes(out,0xB157,0xB294,{0x3D,0x2F,0x00}) ||
       !hasBytes(out,0xB157,0xB294,{0xB8,0x64,0x00,0x50,0xB8,0x01,0x00,0x50})){
        error="OUT.EXE periodic animation rule oracle mismatch"; return false;
    }

    // The world projectile routine EF3 has exactly two normal callers.  Their
    // caller-side constants select physical effect tiles 50 and 49.
    const auto projectileCalls=rel16CallSites(out,0x0EF3);
    if(projectileCalls!=std::vector<int>({0x1268,0x1BF5}) ||
       !hasBytes(out,0x124F,0x1269,{0xBB,0x32,0x00}) ||
       !hasBytes(out,0x1BDC,0x1BF6,{0xBB,0x31,0x00})){
        error="OUT.EXE projectile effect tile inventory mismatch"; return false;
    }

    // Additional direct transient draws of 49/50.
    if(!hasBytes(out,0x1A70,0x1A8A,{0xB9,0x31,0x00,0x51,0xE8,0x2A,0x8D}) ||
       !hasBytes(out,0x8490,0x84AC,{0xBB,0x32,0x00,0x53,0x89,0x4E,0x0E,0xE8,0x09,0x23}) ||
       !hasBytes(out,0x85B2,0x85C2,{0xB8,0x32,0x00,0x50,0xE8,0xF3,0x21})){
        error="OUT.EXE transient effect direct-draw oracle mismatch"; return false;
    }

    // Monster producer: base physical tile = 19 + 2*monsterIndex.  With the
    // fifteen recovered monster kinds, normal producers stop at base 47.
    if(!hasBytes(out,0x819E,0x81A9,{0x8B,0x46,0x14,0xD1,0xE0,0x05,0x13,0x00})){
        error="OUT.EXE monster base-tile producer oracle mismatch"; return false;
    }

    // OUT.EXE 0x90B9: three-BIOS-tick idle quantum, counter increment, B157.
    if(!hasBytes(out,0x90B9,0x90F7,{0xFF,0x46,0x04}) ||
       !hasBytes(out,0x90B9,0x90F7,{0xE8,0x79,0x20}) ||
       !hasBytes(out,0x90B9,0x90F7,{0x81,0x7E,0x04,0xDC,0x05})){
        error="OUT.EXE input-wait animation oracle mismatch"; return false;
    }

    return true;
}

void outline(SDL_Renderer* r,const SDL_Rect& q,SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_RenderDrawRect(r,&q);
}

void fill(SDL_Renderer* r,const SDL_Rect& q,SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_RenderFillRect(r,&q);
}

void text(SDL_Renderer* r,const PixelFont& font,int x,int y,const std::string& s,SDL_Color c=WHITE){
    font.drawFrontend(r,x,y,s,c);
}

std::uint32_t mix(std::uint32_t x){
    x^=x>>16; x*=0x7FEB352Du; x^=x>>15; x*=0x846CA68Bu; x^=x>>16;
    return x;
}

int randomPairFrame(int base,std::uint32_t step){
    // Inspector-only deterministic stand-in for OUT.EXE randomRange(1,100)%2.
    // The physical pair is oracle-verified; gameplay RNG determines the live
    // sequence, so manual stepping uses a stable demonstration sequence.
    return base + static_cast<int>(
        mix(step*0x9E3779B9u + static_cast<std::uint32_t>(base)*0x85EBCA6Bu)&1u);
}

int physicalFrameFor(const AnimGroup& g,std::uint32_t step){
    switch(g.rule){
        case Rule::CastleHold: return ((step%6u)>=3u)?5:4;
        case Rule::TownPulse: return (step%4u==0u)?7:8;
        case Rule::ToggleParity: return g.baseTile + static_cast<int>(step&1u);
        case Rule::RandomPair: return randomPairFrame(g.baseTile,step);
        case Rule::WaterRotate: return 0;
    }
    return g.baseTile;
}

SDL_Texture* textureFor(const AnimGroup& g,const OriginalData& original,std::uint32_t step){
    switch(g.rule){
        case Rule::WaterRotate: return original.animatedTerrainTexture(0,step);
        case Rule::CastleHold: return original.animatedTerrainTexture(4,step);
        case Rule::TownPulse: return original.animatedTerrainTexture(6,step);
        case Rule::ToggleParity:
        case Rule::RandomPair:
            return original.physicalTexture(physicalFrameFor(g,step));
    }
    return nullptr;
}

std::string ruleName(Rule r){
    switch(r){
        case Rule::WaterRotate:return "BITMAP ROTATE";
        case Rule::CastleHold:return "PAIR 4/5 - 3+3 HOLD";
        case Rule::TownPulse:return "PAIR 7/8 - 1+3";
        case Rule::ToggleParity:return "PAIR - EVEN/ODD";
        case Rule::RandomPair:return "PAIR - RANDOM 50/50";
    }
    return "?";
}

TileUse tileUse(int t){
    if(t==0) return TileUse::PeriodicBitmap;
    if(t==4 || t==5 || t==7 || t==8 || t==14 || t==15 || (t>=19 && t<=48))
        return TileUse::PeriodicPair;
    if(t==49 || t==50) return TileUse::TransientFx;
    if(t==51) return TileUse::NoNormalDraw;
    return TileUse::StaticUsed;
}

const char* tileUseShort(TileUse u){
    switch(u){
        case TileUse::PeriodicBitmap:return "IDLE BITMAP";
        case TileUse::PeriodicPair:return "IDLE PAIR";
        case TileUse::StaticUsed:return "STATIC";
        case TileUse::TransientFx:return "TRANSIENT FX";
        case TileUse::NoNormalDraw:return "NO NORMAL DRAW";
    }
    return "?";
}

SDL_Color tileUseColor(TileUse u){
    switch(u){
        case TileUse::PeriodicBitmap:
        case TileUse::PeriodicPair:return CYAN;
        case TileUse::StaticUsed:return WHITE;
        case TileUse::TransientFx:return YELLOW;
        case TileUse::NoNormalDraw:return RED;
    }
    return WHITE;
}

const char* tileRole(int t){
    switch(t){
        case 0:return "WATER";
        case 4:case 5:return "CASTLE";
        case 7:case 8:return "TOWN";
        case 10:return "PLAYER";
        case 11:return "HORSE";
        case 12:return "CART";
        case 13:return "RAFT";
        case 14:case 15:return "FRIGATE";
        case 16:return "AIRCAR";
        case 17:return "SHUTTLE";
        case 18:return "TIME MACHINE";
        case 19:case 20:return "NESS";
        case 21:case 22:return "SQUID";
        case 23:case 24:return "DRAGON TURTLE";
        case 25:case 26:return "PIRATE SHIP";
        case 27:case 28:return "HOOD";
        case 29:case 30:return "BEAR";
        case 31:case 32:return "HIDDEN ARCHER";
        case 33:case 34:return "DARK KNIGHT";
        case 35:case 36:return "EVIL TRENT";
        case 37:case 38:return "THIEF";
        case 39:case 40:return "ORC";
        case 41:case 42:return "KNIGHT";
        case 43:case 44:return "NECROMANCER";
        case 45:case 46:return "EVIL RANGER";
        case 47:case 48:return "WARLOCK";
        case 49:return "BLUE EFFECT";
        case 50:return "RED EFFECT";
        case 51:return "ASSET ONLY";
        default:return "MAP / STATIC";
    }
}

void drawAnimView(SDL_Renderer* r,const PixelFont& font,const OriginalData& original,
                  std::uint32_t step){
    constexpr int startX=16,startY=86,cardW=224,cardH=160,gapX=12,gapY=12;
    for(std::size_t i=0;i<GROUPS.size();++i){
        const int col=static_cast<int>(i%5);
        const int row=static_cast<int>(i/5);
        const int x=startX+col*(cardW+gapX);
        const int y=startY+row*(cardH+gapY);
        SDL_Rect card{x,y,cardW,cardH};
        fill(r,card,BLACK);
        outline(r,card,BLUE);

        const auto& g=GROUPS[i];
        text(r,font,x+8,y+8,g.name,WHITE);

        SDL_Texture* tex=textureFor(g,original,step);
        SDL_Rect tile{x+10,y+38,80,80};
        if(tex) SDL_RenderCopy(r,tex,nullptr,&tile);
        outline(r,tile,CYAN);

        std::ostringstream frame;
        if(g.rule==Rule::WaterRotate) frame<<"WATER PHASE "<<(step&3u);
        else frame<<"VISIBLE TILE "<<physicalFrameFor(g,step);
        text(r,font,x+102,y+42,frame.str(),CYAN);
        text(r,font,x+102,y+70,ruleName(g.rule),YELLOW);

        if(g.rule==Rule::RandomPair)
            text(r,font,x+102,y+100,"PAIR ORACLE",GREY);
    }

    // Tiles 49 and 50 are not periodic frame animation.  They are nevertheless
    // animated graphics in normal play because OUT.EXE moves them cell by cell
    // through EF3 and direct combat draw paths.
    const int y=744;
    text(r,font,16,y,"TRANSIENT EFFECTS:",YELLOW);
    SDL_Rect a{176,y-8,32,32},b{360,y-8,32,32};
    SDL_RenderCopy(r,original.physicalTexture(49),nullptr,&a);
    SDL_RenderCopy(r,original.physicalTexture(50),nullptr,&b);
    text(r,font,216,y,"49 - MOVED BY PROJECTILE PATH",YELLOW);
    text(r,font,400,y,"50 - MOVED BY PROJECTILE/COMBAT PATH",YELLOW);
}

void drawAllTiles(SDL_Renderer* r,const PixelFont& font,const OriginalData& original){
    constexpr int cols=13;
    constexpr int cellW=90,cellH=144;
    constexpr int startX=18,startY=112;

    for(int t=0;t<52;++t){
        const int col=t%cols,row=t/cols;
        const int x=startX+col*cellW,y=startY+row*cellH;
        const TileUse use=tileUse(t);
        const SDL_Color c=tileUseColor(use);

        SDL_Rect tile{x,y,64,64};
        if(auto* tex=original.physicalTexture(t)) SDL_RenderCopy(r,tex,nullptr,&tile);
        outline(r,tile,c);

        std::ostringstream n;
        n<<"T"<<t<<" "<<tileRole(t);
        text(r,font,x,y+70,n.str(),c);
        text(r,font,x,y+92,tileUseShort(use),c);
    }

    text(r,font,18,706,"COMPLETE EGATILES.BIN INVENTORY: 52 PHYSICAL TILES",WHITE);
    text(r,font,18,730,"37 PERIODIC MEMBERS  |  12 STATIC USED  |  2 TRANSIENT EFFECTS  |  1 NO NORMAL OUT.EXE DRAW PATH",CYAN);
    text(r,font,18,754,"PERIODIC = B157 IDLE DISPATCH.  TRANSIENT 49/50 MOVE DURING ATTACKS BUT DO NOT FRAME-CYCLE.",GREY);
}

void updateTitle(SDL_Window* win,bool paused,bool allTiles,std::uint32_t step){
    std::ostringstream ss;
    ss<<"Ultima I Complete EGATILES Animation Audit - "
      <<(allTiles?"ALL 52 CLASSIFIED":"19 PERIODIC GROUPS + TRANSIENT FX")
      <<" - STEP "<<step<<" - "<<(paused?"PAUSED":"RUNNING")
      <<" - SPACE pause | LEFT/RIGHT step | A view | R reset | ESC quit";
    SDL_SetWindowTitle(win,ss.str().c_str());
}

} // namespace

int main(int argc,char** argv){
    std::string data="gamedata";
    std::string h12="fonts/H12.FNT";
    std::string robo="fonts/ROBO.FNT";
    bool startPaused=false;

    for(int i=1;i<argc;++i){
        const std::string a=argv[i];
        if(a=="--data" && i+1<argc) data=argv[++i];
        else if(a=="--h12" && i+1<argc) h12=argv[++i];
        else if(a=="--robo" && i+1<argc) robo=argv[++i];
        else if(a=="--paused") startPaused=true;
        else{
            std::cerr<<"usage: u1kr_tile_inspector [--data DIR] [--h12 FILE] [--robo FILE] [--paused]\n";
            return 2;
        }
    }

    std::string oracleError;
    if(!verifyAnimationOracle(data,oracleError)){
        std::cerr<<"Complete EGATILES audit failed: "<<oracleError<<"\n";
        return 3;
    }

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){
        std::cerr<<SDL_GetError()<<"\n"; return 4;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");

    SDL_Window* win=SDL_CreateWindow(
        "Ultima I Complete EGATILES Animation Audit",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIN_W,WIN_H,0);
    if(!win){ std::cerr<<SDL_GetError()<<"\n"; SDL_Quit(); return 5; }

    SDL_Renderer* r=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!r) r=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);
    if(!r){ std::cerr<<SDL_GetError()<<"\n"; SDL_DestroyWindow(win); SDL_Quit(); return 6; }

    PixelFont font;
    if(!font.load(h12,robo)){
        std::cerr<<"Could not load H12.FNT / ROBO.FNT\n";
        SDL_DestroyRenderer(r); SDL_DestroyWindow(win); SDL_Quit(); return 7;
    }

    OriginalData original;
    std::string error;
    if(!original.load(data,r,error)){
        std::cerr<<"Could not load Ultima I data: "<<error<<"\n";
        SDL_DestroyRenderer(r); SDL_DestroyWindow(win); SDL_Quit(); return 8;
    }

    bool running=true;
    bool paused=startPaused;
    bool allTiles=true; // M81: start with the exhaustive 0..51 classification.
    std::uint32_t step=0;
    std::uint32_t epoch=SDL_GetTicks();

    while(running){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT){ running=false; break; }
            if(e.type!=SDL_KEYDOWN) continue;
            const SDL_Keycode k=e.key.keysym.sym;
            if(k==SDLK_ESCAPE){ running=false; break; }
            if(k==SDLK_a){ allTiles=!allTiles; }
            else if(k==SDLK_r){ step=0; epoch=SDL_GetTicks(); paused=false; }
            else if(k==SDLK_SPACE){
                if(paused){
                    epoch=SDL_GetTicks()-step*ANIM_MS;
                    paused=false;
                }else{
                    step=(SDL_GetTicks()-epoch)/ANIM_MS;
                    paused=true;
                }
            }else if(k==SDLK_LEFT){
                if(!paused) step=(SDL_GetTicks()-epoch)/ANIM_MS;
                paused=true;
                if(step>0) --step;
            }else if(k==SDLK_RIGHT){
                if(!paused) step=(SDL_GetTicks()-epoch)/ANIM_MS;
                paused=true;
                ++step;
            }
        }

        if(!paused) step=(SDL_GetTicks()-epoch)/ANIM_MS;
        updateTitle(win,paused,allTiles,step);

        SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,BLACK.a);
        SDL_RenderClear(r);

        text(r,font,16,12,"ULTIMA I COMPLETE EGATILES.BIN ANIMATION AUDIT",WHITE);
        text(r,font,16,36,"ORACLE VERIFIED: ALL 20 A7B4 BLITTER CALLS + A754 + B157 + EF3",CYAN);
        text(r,font,16,60,
             allTiles
                ?"A: SHOW PERIODIC GROUPS        SPACE: PAUSE   LEFT/RIGHT: STEP   R: RESET   ESC: QUIT"
                :"A: SHOW ALL 52 CLASSIFIED      SPACE: PAUSE   LEFT/RIGHT: STEP   R: RESET   ESC: QUIT",
             GREY);

        if(allTiles) drawAllTiles(r,font,original);
        else drawAnimView(r,font,original,step);

        SDL_RenderPresent(r);
        SDL_Delay(4);
    }

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}