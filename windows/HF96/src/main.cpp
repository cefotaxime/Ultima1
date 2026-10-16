#ifdef __DJGPP__
extern "C" void hf93AudioCaptureTick(void);
#endif
#include <cstring>
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <deque>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include "OriginalData.h"
#include "NameEncoding.h"
#include "LocalSimulation.h"
#include "ShopSystem.h"
#include "KingSystem.h"
#include "VehicleSystem.h"
#include "OverworldSimulation.h"
#include "ResurrectionSystem.h"
#include "DungeonSystem.h"
#include "SpaceSystem.h"
#include "CommandSystem.h"
#include "MondainSystem.h"
#include "SfxSystem.h"
#include "PixelFont.h"
#include "FrontendSystem.h"
#include "KoreanText.h"
#include "CastleCommandText.h"
#include "ZtatsModel.h"
#include "QuitSystem.h"
#include "CastleLabelOverlay.h"
#include "TownLabelOverlay.h"
#include "GameplayTextWrap.h"
#include "WorldTileAnimation.h"
#include "VehicleFacing.h"
#include "LocalActorDialoguePresentation.h"
#include "PrincessRescuePresentation.h"
#include "LandmarkPresentation.h"
#include "AttackPresentation.h"
#include "DeathSkullData.h"

namespace {
constexpr int W=640,H=400;
// Ultima I renders a 320x200 raster but was intended for a 4:3 display.
// Our clean-room logical canvas is the exact 2x raster (640x400).  Present it
// directly as an 800x600 4:3 window so the DOS pixel aspect ratio is restored
// without pillarboxes.
constexpr int WINDOW_W=800,WINDOW_H=600;
constexpr int PRESENT_W=800,PRESENT_H=600;
constexpr int PRESENT_X=0;
constexpr int PRESENT_Y=0;
constexpr std::uint32_t dosTicksToMs(std::uint32_t ticks){
    // Same BIOS timer quantum used by the original OUT.EXE input wait.
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(ticks)*65536u*1000u + 1193181u)/1193182u);
}
constexpr std::uint32_t OVERWORLD_ANIM_MS=dosTicksToMs(3);
constexpr std::uint32_t OUT_IDLE_AUTOPASS_MS=dosTicksToMs(3u*1500u); // OUT.EXE 0x90B9
constexpr std::uint32_t WORLD_PROJECTILE_STEP_MS=dosTicksToMs(WORLD_PROJECTILE_DOS_TICKS);
constexpr int VIEW_X=16,VIEW_Y=16;
constexpr int WORLD_TILE=32,WORLD_COLS=19,WORLD_ROWS=9;
constexpr int LOCAL_TILE=16;
constexpr SDL_Color BLACK{0,0,0,255};
constexpr SDL_Color WHITE{255,255,255,255};
constexpr SDL_Color EGA_BLUE{0,0,170,255};
constexpr SDL_Color EGA_LIGHT_CYAN{85,255,255,255};
constexpr SDL_Color EGA_LIGHT_RED{255,85,85,255};

void rect(SDL_Renderer* r,SDL_Rect rr,SDL_Color c){SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);SDL_RenderDrawRect(r,&rr);}

// OUT.EXE 0xAD9D draws a 64x95 monochrome skull from DS:1A14 at source
// coordinate (128,31).  The clean-room canvas is the exact 2x 320x200 raster.
void drawResurrectionSkull(SDL_Renderer* r){
    // OUT.EXE 0xC69F clears source rectangle (0,8)..(311,151) to black before
    // drawing the skull. drawFrames() is called later and restores frame rails.
    SDL_Rect clear{0,16,624,288};
    SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,BLACK.a);
    SDL_RenderFillRect(r,&clear);

    SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,WHITE.a);
    for(int y=0;y<95;++y){
        for(int word=0;word<4;++word){
            const std::uint16_t bits=DEATH_SKULL_BITMAP[static_cast<std::size_t>(y)][static_cast<std::size_t>(word)];
            for(int bit=0;bit<16;++bit){
                if((bits & static_cast<std::uint16_t>(0x8000u>>bit))==0) continue;
                SDL_Rect px{2*(128+word*16+bit),2*(31+y),2,2};
                SDL_RenderFillRect(r,&px);
            }
        }
    }
}


void drawCastleZoneLabels(SDL_Renderer* r,const PixelFont& font,int mapIndex){
    for(const auto& label:CASTLE_ZONE_LABELS){
        if(label.mapIndex!=mapIndex) continue;

        const int textW=font.measure(label.korean);

        // Preserve the center of the original English tile-string exactly.
        // This also handles F/W/A: the longer Korean text grows equally left/right
        // around the original one-letter center instead of drifting.
        const int x=castleZoneLabelCenteredX(label,textW,VIEW_X,LOCAL_TILE);
        const int y=VIEW_Y+label.y*LOCAL_TILE;
        font.draw(r,x,y,label.korean,BLACK);
    }
}

std::vector<std::string> utf8Codepoints(const std::string& text){
    std::vector<std::string> out;
    for(std::size_t i=0;i<text.size();){
        const unsigned char c=static_cast<unsigned char>(text[i]);
        std::size_t n=(c<0x80)?1:((c&0xE0)==0xC0?2:((c&0xF0)==0xE0?3:4));
        n=std::min(n,text.size()-i);
        out.push_back(text.substr(i,n));
        i+=n;
    }
    return out;
}

void drawTownWallLabels(SDL_Renderer* r,const PixelFont& font,int mapIndex){
    forEachTownWallLabel([&](const TownWallLabel& label){
        if(label.mapIndex!=mapIndex) return;

        // M166: the inverse-white treatment belongs only to cells that actually
        // contain a Korean glyph.  The recovered English span is erased back to
        // the surrounding wall by the map renderer; do NOT repaint that whole
        // span white here.  Each Hangul glyph gets exactly one 8x8 source tile
        // (16x16 presentation) of white background, matching the original
        // inverse character-tile convention without creating a long white bar.
        const auto glyphs=utf8Codepoints(label.korean);
        if(label.y0==label.y1){
            const int spanX=VIEW_X+label.x0*LOCAL_TILE;
            const int spanW=(label.x1-label.x0+1)*LOCAL_TILE;
            const int totalW=static_cast<int>(glyphs.size())*LOCAL_TILE;
            const int startX=spanX+(spanW-totalW)/2;
            const int y=VIEW_Y+label.y0*LOCAL_TILE;
            for(std::size_t i=0;i<glyphs.size();++i){
                const int bx=startX+static_cast<int>(i)*LOCAL_TILE;
                SDL_Rect cellBg{bx,y,LOCAL_TILE,LOCAL_TILE};
                SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,WHITE.a);
                SDL_RenderFillRect(r,&cellBg);
                const int gw=font.measure(glyphs[i]);
                font.draw(r,bx+(LOCAL_TILE-gw)/2,y,glyphs[i],BLACK);
            }
        }else{
            const int spanY=VIEW_Y+label.y0*LOCAL_TILE;
            const int spanH=(label.y1-label.y0+1)*LOCAL_TILE;
            const int totalH=static_cast<int>(glyphs.size())*LOCAL_TILE;
            const int startY=spanY+(spanH-totalH)/2;
            const int x=VIEW_X+label.x0*LOCAL_TILE;
            for(std::size_t i=0;i<glyphs.size();++i){
                const int by=startY+static_cast<int>(i)*LOCAL_TILE;
                SDL_Rect cellBg{x,by,LOCAL_TILE,LOCAL_TILE};
                SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,WHITE.a);
                SDL_RenderFillRect(r,&cellBg);
                const int gw=font.measure(glyphs[i]);
                font.draw(r,x+(LOCAL_TILE-gw)/2,by,glyphs[i],BLACK);
            }
        }
    });
}

struct LogRow {
    std::string text;
    bool prompt=false;
};

struct Log {
    std::deque<LogRow> rows;
    void push(std::string s,bool prompt=false){
        if(s.empty()) return;
        rows.push_back(LogRow{std::move(s),prompt});
        while(rows.size()>3) rows.pop_front();
    }
    void pushCommand(std::string s){ push(std::move(s),true); }
    void appendLast(const std::string& suffix){
        if(rows.empty()) return;
        rows.back().text+=suffix;
    }
    void replaceLastCommand(std::string s){
        if(rows.empty()){ pushCommand(std::move(s)); return; }
        rows.back().text=std::move(s);
        rows.back().prompt=true;
    }
};

std::string statusValue(std::uint16_t v){
    // OUT.EXE 0xAB66 is called by the HUD with width=4 and pad=' '.
    // Values are clamped to 9999 by the individual status routines.
    const unsigned n=std::min<unsigned>(v,9999u);
    std::string t=std::to_string(n);
    if(t.size()<4) t.insert(t.begin(),4-t.size(),' ');
    return t;
}

std::string terrainInfo(int t){
    // Deliberately English until its exact DOS wording is individually reviewed.
    switch(t){
        case 0:return "You are at sea";
        case 2:return "You are in the woods";
        default:return "You are in the lands";
    }
}

bool asciiLegacyName(const std::string& s){
    if(s.empty() || s.size()>15) return false;
    return std::all_of(s.begin(),s.end(),[](unsigned char c){ return c>=32 && c<=126; });
}
bool writeNameSidecar(const std::string& path,const std::string& name){
    const std::string side=path+".krname";
    if(asciiLegacyName(name)){ std::remove(side.c_str()); return true; }
    std::ofstream f(side,std::ios::binary|std::ios::trunc);
    if(!f) return false;
    f.write(name.data(),static_cast<std::streamsize>(name.size()));
    return static_cast<bool>(f);
}
std::string readNameSidecar(const std::string& path){
    std::ifstream f(path+".krname",std::ios::binary);
    if(!f) return {};
    return std::string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
}

bool saveState(const std::string& path, const PlayerSave& p){
    std::fstream f(path,std::ios::in|std::ios::out|std::ios::binary);
    if(!f) return false;
    auto wr=[&](std::streamoff off,std::uint16_t v){
        char b[2]={static_cast<char>(v&0xFF),static_cast<char>((v>>8)&0xFF)};
        f.seekp(off); f.write(b,2);
    };
    wr(0x16,static_cast<std::uint16_t>(std::min<unsigned>(p.hits,9999u)));
    wr(0x18,p.strength); wr(0x1A,p.agility); wr(0x1C,p.stamina);
    wr(0x1E,p.charisma); wr(0x20,p.wisdom); wr(0x22,p.intelligence);
    wr(0x24,static_cast<std::uint16_t>(std::min<unsigned>(p.coin,9999u)));
    wr(0x26,static_cast<std::uint16_t>(std::min<unsigned>(p.experience,9999u)));
    wr(0x28,static_cast<std::uint16_t>(std::min<unsigned>(p.food,9999u)));
    wr(0x2A,p.readyWeapon); wr(0x2C,p.readySpell); wr(0x2E,p.readyArmour); wr(0x30,p.transport);
    wr(0x34,p.x); wr(0x36,p.y);
    for(int i=0;i<9;++i) wr(0x3A+i*2,p.kingQuest[static_cast<std::size_t>(i)]);
    for(int i=0;i<4;++i) wr(0x4C+i*2,p.gems[static_cast<std::size_t>(i)]);
    for(int i=0;i<6;++i)  wr(0x54+i*2,p.armour[static_cast<std::size_t>(i)]);
    for(int i=0;i<16;++i) wr(0x60+i*2,p.weapons[static_cast<std::size_t>(i)]);
    for(int i=0;i<11;++i) wr(0x80+i*2,p.spells[static_cast<std::size_t>(i)]);
    for(int i=0;i<8;++i)  wr(0x96+i*2,p.transports[static_cast<std::size_t>(i)]);
    wr(0xA6,p.spaceKills);
    wr(0xAC,static_cast<std::uint16_t>(p.steps&0xFFFFu));
    wr(0xAE,static_cast<std::uint16_t>((p.steps>>16)&0xFFFFu));
    wr(0xB0,p.shuttleStat0); wr(0xB2,p.shuttleStat1);
    for(int i=0;i<OriginalData::WORLD_OBJECT_COUNT;++i)
        for(int j=0;j<8;++j) wr(0xB4+i*16+j*2,p.worldObjects[static_cast<std::size_t>(i)].w[static_cast<std::size_t>(j)]);
    f.flush();
    return static_cast<bool>(f);
}


bool saveNewCharacter(const std::string& path,const PlayerSave& p){
    // GEN.EXE writes a fresh 820-byte PLAYER*.U1 record from DS:A30.
    // This reconstructs the same initial layout rather than carrying unknown
    // bytes over from a previous character in the selected host save slot.
    std::array<std::uint8_t,820> b{};
    auto wr=[&](std::size_t off,std::uint16_t v){ b[off]=static_cast<std::uint8_t>(v&255); b[off+1]=static_cast<std::uint8_t>(v>>8); };
    const std::string legacyName=NameEncoding::toCp949(p.name);
    if(legacyName.empty() || legacyName.size()>14) return false;
    const auto n=std::min<std::size_t>(14,legacyName.size());
    for(std::size_t i=0;i<n;++i) b[i]=static_cast<std::uint8_t>(legacyName[i]);
    wr(0x10,p.race); wr(0x12,p.klass); wr(0x14,p.sex);
    wr(0x16,static_cast<std::uint16_t>(std::min<unsigned>(p.hits,9999u))); wr(0x18,p.strength); wr(0x1A,p.agility); wr(0x1C,p.stamina);
    wr(0x1E,p.charisma); wr(0x20,p.wisdom); wr(0x22,p.intelligence);
    wr(0x24,static_cast<std::uint16_t>(std::min<unsigned>(p.coin,9999u)));
    wr(0x26,static_cast<std::uint16_t>(std::min<unsigned>(p.experience,9999u)));
    wr(0x28,static_cast<std::uint16_t>(std::min<unsigned>(p.food,9999u)));
    wr(0x2A,p.readyWeapon); wr(0x2C,p.readySpell); wr(0x2E,p.readyArmour); wr(0x30,p.transport);
    wr(0x32,p.rngSeedWord); wr(0x34,p.x); wr(0x36,p.y);
    // GEN.EXE 0x13D5 seeds PLAYER+0x38 with 1; quest/gem words are zeroed.
    wr(0x38,1);
    for(int i=0;i<9;++i) wr(0x3A+i*2,p.kingQuest[static_cast<std::size_t>(i)]);
    for(int i=0;i<4;++i) wr(0x4C+i*2,p.gems[static_cast<std::size_t>(i)]);
    for(int i=0;i<6;++i) wr(0x54+i*2,p.armour[static_cast<std::size_t>(i)]);
    for(int i=0;i<16;++i) wr(0x60+i*2,p.weapons[static_cast<std::size_t>(i)]);
    for(int i=0;i<11;++i) wr(0x80+i*2,p.spells[static_cast<std::size_t>(i)]);
    for(int i=0;i<8;++i) wr(0x96+i*2,p.transports[static_cast<std::size_t>(i)]);
    wr(0xA6,p.spaceKills); wr(0xA8,0xFFFF); // GEN.EXE 0x14B4 sentinel
    wr(0xAC,static_cast<std::uint16_t>(p.steps&0xFFFFu)); wr(0xAE,static_cast<std::uint16_t>(p.steps>>16));
    wr(0xB0,p.shuttleStat0); wr(0xB2,p.shuttleStat1);
    for(int i=0;i<OriginalData::WORLD_OBJECT_COUNT;++i)
        for(int j=0;j<8;++j) wr(0xB4+i*16+j*2,p.worldObjects[static_cast<std::size_t>(i)].w[static_cast<std::size_t>(j)]);
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    if(!f) return false;
    f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));
    f.flush();
    if(!f) return false;
    std::remove((path+".krname").c_str());
    return true;
}

std::string savedCharacterName(const std::string& path){
    if(const auto ext=readNameSidecar(path); !ext.empty()) return ext;
    std::ifstream f(path,std::ios::binary);
    if(!f) return {};
    char b[15]{}; f.read(b,15);
    if(f.gcount()!=15) return {};
    std::size_t n=0; while(n<15 && b[n]!=0) ++n;
    return NameEncoding::fromSave(std::string_view(b,n));
}

void fillRect(SDL_Renderer* r,SDL_Rect rr,SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); SDL_RenderFillRect(r,&rr);
}

void fillSourceRect(SDL_Renderer* r,int x,int y,int w,int h,SDL_Color c){
    fillRect(r,{x*2,y*2,w*2,h*2},c);
}

void drawSourcePoint(SDL_Renderer* r,int x,int y,SDL_Color c){
    fillSourceRect(r,x,y,1,1,c);
}

void drawFrames(SDL_Renderer* r){
    // M85 reconstructs the 320x200 EGA frame itself instead of approximating
    // it with five host rectangles.  There is deliberately NO white outline
    // on the outside edge of the screen.

    // Rounded blue upper frame.  The first seven source scanlines step inward
    // at the two upper corners exactly as visible in the original EGA output.
    static constexpr int topInset[7]={7,5,4,3,2,2,1};
    for(int y=0;y<7;++y){
        const int inset=topInset[y];
        fillSourceRect(r,inset,y,320-2*inset,1,EGA_BLUE);
    }

    // Blue side rails around the 304-pixel world viewport.
    fillSourceRect(r,0,7,7,145,EGA_BLUE);       // x 0..6
    fillSourceRect(r,313,7,7,145,EGA_BLUE);     // x 313..319

    // Blue lower rail below the viewport; source y159 belongs to the white
    // lower-pane border, not to this band.
    fillSourceRect(r,0,152,320,7,EGA_BLUE);     // y 152..158

    // Lower text/status separator: original capture has a four-pixel blue
    // gutter between white vertical rules at x241 and x246.
    fillSourceRect(r,242,160,4,40,EGA_BLUE);    // x 242..245

    // Main viewport inner white border.  Each corner pixel is intentionally
    // left blank: horizontal and vertical segments never share an endpoint.
    fillSourceRect(r,8,7,304,1,WHITE);          // top:    x 8..311
    fillSourceRect(r,7,8,1,143,WHITE);          // left:   y 8..150
    fillSourceRect(r,312,8,1,143,WHITE);        // right:  y 8..150
    fillSourceRect(r,8,151,304,1,WHITE);        // bottom: x 8..311

    // Command window: white top and right inner rules.  Unlike the main
    // viewport border, this lower-pane corner is solid in the original.
    fillSourceRect(r,0,159,242,1,WHITE);         // x 0..241, includes corner
    fillSourceRect(r,241,160,1,40,WHITE);        // y 160..199

    // Status window: white left and top inner rules with a filled shared
    // corner at (246,159).  No outside/bottom white border is drawn.
    fillSourceRect(r,246,159,74,1,WHITE);        // x 246..319, includes corner
    fillSourceRect(r,246,160,1,40,WHITE);        // y 160..199
}


void drawTimeMachineUnreadyPopup(SDL_Renderer* r,const PixelFont& font,bool active){
    if(!active) return;
    // OUT.EXE 0x14A1 calls shared popup helper 0x23C2, which clears
    // source (31,23)..(287,127) and draws the 0x2368 blue outline
    // at (34,26)..(284,124). The source text is written on rows 7/8/9.
    fillSourceRect(r,31,23,257,105,BLACK);
    fillSourceRect(r,34,26,251,1,EGA_BLUE);
    fillSourceRect(r,284,26,1,99,EGA_BLUE);
    fillSourceRect(r,34,124,251,1,EGA_BLUE);
    fillSourceRect(r,34,26,1,99,EGA_BLUE);

    const int centerX=159*2;
    const std::array<std::string,3> lines={
        "당신은 아직",
        "기체를 작동하는 방법을",
        "알지 못합니다."
    };
    for(int i=0;i<3;++i){
        const int w=font.measure(lines[static_cast<std::size_t>(i)]);
        font.draw(r,centerX-w/2,(7+i)*16,lines[static_cast<std::size_t>(i)],EGA_LIGHT_CYAN);
    }
}

void drawKingServicePopup(SDL_Renderer* r,const PixelFont& font,const KingSystem& king){
    if(king.stage()!=KingStage::ServicePopup) return;

    // OUT.EXE 0x23C2 first clears source rectangle (31,23)..(287,127).
    fillSourceRect(r,31,23,257,105,BLACK);

    // 0x2368 then draws the blue outline:
    // (34,26)->(284,26)->(284,124)->(34,124)->(34,26).
    fillSourceRect(r,34,26,251,1,EGA_BLUE);
    fillSourceRect(r,284,26,1,99,EGA_BLUE);
    fillSourceRect(r,34,124,251,1,EGA_BLUE);
    fillSourceRect(r,34,26,1,99,EGA_BLUE);

    const int centerX=159*2;
    const auto& lines=king.popupLines();

    if(king.rewardPopup()){
        // OUT.EXE 0x7AAF -> 0x23C2 opens the same framed popup before the
        // completed-quest greeting and reward are rendered.  The English
        // original uses rows 5/6 for the greeting and rows 8+ for the reward.
        // Keep that vertical structure, but wrap the approved Korean strings
        // by their actual PixelFont width so they remain inside the frame.
        auto drawCenteredWrapped=[&](const std::string& text,int& row){
            const auto wrapped=wrapGameplayTextMeasured(
                text,27*16,[&](const std::string& t){ return font.measure(t); });
            for(const auto& line:wrapped){
                if(row>13) break;
                const int w=font.measure(line);
                font.draw(r,centerX-w/2,row*16,line,EGA_LIGHT_CYAN);
                ++row;
            }
        };

        if(!lines.empty()){
            int greetingRow=5;
            drawCenteredWrapped(lines.front(),greetingRow);
        }
        int rewardRow=8;
        for(std::size_t i=1;i<lines.size() && rewardRow<=13;++i){
            drawCenteredWrapped(lines[i],rewardRow);
            ++rewardRow; // preserve the original blank separation between blocks
        }
        return;
    }

    // Original service text uses BIOS rows 6, 8, 10 and 11.  The Korean
    // "Do not return..." translation fits on one line, so it occupies row 10.
    static constexpr int ROWS[4]={6,8,10,11};
    for(std::size_t i=0;i<lines.size() && i<4;++i){
        const int w=font.measure(lines[i]);
        font.draw(r,centerX-w/2,ROWS[i]*16,lines[i],EGA_LIGHT_CYAN);
    }
}


void drawDungeonFramedLabel(SDL_Renderer* r,const PixelFont& font,
                            const std::string& label,int y,bool upperFrame){
    // M182: reproduce the dungeon frame-title geometry in the original
    // 320x200 source-pixel space.  The previous host-pixel chevrons were only
    // one physical pixel thick, i.e. half the thickness of an EGA source pixel
    // in this 2x renderer.
    //
    // Upper title (source y=0..7): the inward chevrons terminate on the inner
    // viewport border at y=7.  There is no white outer/top border.
    //
    // Lower title (source y=151..159): the title interrupts BOTH the viewport
    // bottom border (y=151) and the command-window top border (y=159).  The two
    // white horizontal rules reconnect only to the *outer* endpoints of the
    // chevrons; neither rule is allowed to run underneath the black title slot.
    const int labelW=font.measure(label);
    constexpr int sidePad=2*8; // exactly two ASCII-space widths
    const int slotW=labelW+sidePad*2;
    const int slotX=(640-slotW)/2;
    const int leftCapX=slotX-16;
    const int rightCapX=slotX+slotW;

    // The lower framed title spans source y151..159 inclusive (9 source rows),
    // because those are the two white frame rules surrounding the blue rail.
    // The upper title spans source y0..7 (8 rows).
    const int clearH=upperFrame ? 16 : 18;
    SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,255);
    SDL_Rect clear{slotX,y,slotW,clearH};
    SDL_RenderFillRect(r,&clear);

    auto sourcePixel=[&](int hx,int hy,SDL_Color c){
        SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
        SDL_Rect q{hx,hy,2,2};
        SDL_RenderFillRect(r,&q);
    };
    auto sourceLine=[&](int x0,int y0,int x1,int y1,SDL_Color c){
        // Coordinates are host coordinates on the even 2x source-pixel grid.
        int sx0=x0/2, sy0=y0/2, sx1=x1/2, sy1=y1/2;
        const int dx=std::abs(sx1-sx0), stepx=(sx0<sx1)?1:-1;
        const int dy=-std::abs(sy1-sy0), stepy=(sy0<sy1)?1:-1;
        int err=dx+dy;
        for(;;){
            sourcePixel(sx0*2,sy0*2,c);
            if(sx0==sx1 && sy0==sy1) break;
            const int e2=2*err;
            if(e2>=dy){ err+=dy; sx0+=stepx; }
            if(e2<=dx){ err+=dx; sy0+=stepy; }
        }
    };

    auto cap=[&](int bx,bool pointsRight){
        const int capH=upperFrame ? 16 : 18;
        SDL_SetRenderDrawColor(r,EGA_BLUE.r,EGA_BLUE.g,EGA_BLUE.b,255);
        SDL_Rect bg{bx,y,16,capH};
        SDL_RenderFillRect(r,&bg);

        if(upperFrame){
            // One EGA source pixel == 2x2 host pixels.  Draw the two diagonal
            // strokes in source space so the white cap has the same thickness
            // as the ordinary viewport border.
            const int outerX=pointsRight ? bx+8 : bx+6;
            const int tipX  =pointsRight ? bx+14: bx;
            sourceLine(outerX,y,tipX,y+6,WHITE);
            sourceLine(tipX,y+6,outerX,y+14,WHITE);

            // source y=7 inner viewport border reconnects to the outer half.
            if(pointsRight) fillRect(r,{bx,y+14,8,2},WHITE);
            else            fillRect(r,{bx+8,y+14,8,2},WHITE);
        }else{
            // Lower cap covers source y151..159 inclusive.  Its upper and
            // lower endpoints meet the two horizontal white frame rules, while
            // the black title slot remains open between the inward tips.
            const int outerX=pointsRight ? bx+8 : bx+6;
            const int tipX  =pointsRight ? bx+14: bx;
            sourceLine(outerX,y,tipX,y+8,WHITE);
            sourceLine(tipX,y+8,outerX,y+16,WHITE);

            // Reconnect BOTH surrounding frame rules only on the outer half.
            // This removes the M181 blue gap before >/< and also prevents the
            // command-window top rule from crossing below the title text.
            if(pointsRight){
                fillRect(r,{bx,y,8,2},WHITE);
                fillRect(r,{bx,y+16,8,2},WHITE);
            }else{
                fillRect(r,{bx+8,y,8,2},WHITE);
                fillRect(r,{bx+8,y+16,8,2},WHITE);
            }
        }
    };

    cap(leftCapX,true);
    cap(rightCapX,false);
    font.draw(r,slotX+sidePad,y,label,EGA_LIGHT_CYAN);
}

struct ShopTitleLineM149 { const char* text; int col; int row; };

std::vector<ShopTitleLineM149> originalShopTitleM149(ShopKind kind,int family){
    // Korean shop-name localization.  Every title is intentionally one line;
    // drawShopPopupM149 centers it independently of the old DOS column data.
    family=std::clamp(family,0,7);
    switch(kind){
        case ShopKind::Transport:
            switch(family){
                case 0:return {{"스쿠터의 초특급 운송상회",0,4}};
                case 3:return {{"최고급 운송회사",0,4}};
                case 5:return {{"신상중고 운송사",0,4}};
                case 7:return {{"꾀돌이 샘의 운송 전문점",0,4}};
                default:return {};
            }
        case ShopKind::Armour:
            switch(family){
                case 0:return {{"엘드릭 드 샤보뇌 경의 방어구점",0,4}};
                case 1:return {{"맥스네 방어구",0,4}};
                case 2:return {{"쇠주먹",0,4}};
                case 3:return {{"망치와 모루",0,4}};
                case 5:return {{"무한 방어 전문점",0,4}};
                case 7:return {{"맞춤 병기 제작소",0,4}};
                default:return {};
            }
        case ShopKind::Weapon:
            switch(family){
                case 0:return {{"엘드릭 드 샤보뇌 경의 무기점",0,4}};
                case 1:return {{"단련된 강철",0,4}};
                case 2:return {{"면도날",0,4}};
                case 3:return {{"차가운 강철 제작소",0,4}};
                case 4:return {{"피 묻은 칼날",0,4}};
                case 5:return {{"결투용품점",0,4}};
                case 6:return {{"무기 보급소",0,4}};
                case 7:return {{"차가운 강철 제작소",0,4}};
            }
            break;
        case ShopKind::Magic:
            switch(family){
                case 0:return {{"광인 채드의 마법상점",0,4}};
                case 1:return {{"신비한 멜린다의 가게",0,4}};
                case 2:return {{"권능어",0,4}};
                case 3:return {{"영매 샘의 마법 백화점",0,4}};
                case 7:return {{"해방된 주문",0,4}};
                default:return {};
            }
        case ShopKind::Food:
            switch(family){
                case 0:return {{"꼬마 카렐리아의 핀란드 식료품점",0,4}};
                case 1:return {{"모험가 보급품",0,4}};
                case 3:return {{"도시락",0,4}};
                case 4:return {{"신선식품 시장",0,4}};
                case 5:return {{"무제한 휴대 식량",0,4}};
                case 6:return {{"신선 패스트푸드 시장",0,4}};
                case 7:return {{"탐험 장비상",0,4}};
                default:return {};
            }
        case ShopKind::Pub:
            switch(family){
                case 0:return {{"바르그 주점",0,4}};
                case 1:return {{"옛날 동네 주점",0,4}};
                case 2:return {{"고양이 박사의 굴",0,4}};
                case 3:return {{"티저의 집",0,4}};
                case 5:return {{"메리네 중심지",0,4}};
                case 6:return {{"커트네 구석 식탁",0,4}};
                case 7:return {{"데이브의 맥주집",0,4}};
                default:return {};
            }
        default: break;
    }
    return {};
}

int originalShopBaseRowM149(ShopKind kind,int family){
    family=std::clamp(family,0,7);
    // Popup line 2 is BIOS row 4; line 3 stays blank; item list starts
    // on popup line 4 (BIOS row 6), for every shop category.
    (void)kind; (void)family;
    return 6;
}

void drawShopPopupM149(SDL_Renderer* r,const PixelFont& font,const ShopSystem& shop,const PlayerSave& p){
    // The original does not open a shop window until Buy/Sell has been chosen.
    if(!shop.active() || shop.stage()==TradeStage::BuySell) return;

    // All town shops call the same OUT.EXE 0x23C2 frame helper.
    fillSourceRect(r,31,23,257,105,BLACK);
    fillSourceRect(r,34,26,251,1,EGA_BLUE);
    fillSourceRect(r,284,26,1,99,EGA_BLUE);
    fillSourceRect(r,34,124,251,1,EGA_BLUE);
    fillSourceRect(r,34,26,1,99,EGA_BLUE);

    // Localized shop-name presentation: one line only, horizontally centered.
    // The 0x23C2 popup begins in BIOS row 3, so row 4 is its second text line.
    // Row 5 is deliberately blank; the item list starts on row 6.
    const int popupCenterX=159*2;
    for(const auto& t:originalShopTitleM149(shop.kind(),shop.townFamily())){
        const std::string title=t.text;
        const int x=popupCenterX-font.measure(title)/2;
        font.draw(r,x,4*16,title,EGA_LIGHT_CYAN);
    }

    if(shop.stage()==TradeStage::Message){
        int row=8;
        for(const auto& line:shop.popupMessage()){
            const int x=popupCenterX-font.measure(line)/2;
            font.draw(r,x,row*16,line,EGA_LIGHT_CYAN);
            ++row;
        }
        return;
    }

    const int base=originalShopBaseRowM149(shop.kind(),shop.townFamily());
    if(shop.stage()==TradeStage::FoodQuantity){
        // Original food shop uses the same popup for the pack-price question.
        // Keep the approved Korean sentence, split only for the popup width.
        const std::string a="식량 10인분 꾸러미 하나 가격은 "+std::to_string(ShopSystem::foodPackPrice(p))+" 코인입니다.";
        const std::string b="몇 꾸러미 사시겠습니까?";
        font.draw(r,popupCenterX-font.measure(a)/2,(base+2)*16,a,EGA_LIGHT_CYAN);
        font.draw(r,popupCenterX-font.measure(b)/2,(base+3)*16,b,EGA_LIGHT_CYAN);
        return;
    }

    int row=base;
    for(const auto& o:shop.options()){
        std::string left;
        left.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(o.key))));
        left += ") "+o.name;
        font.draw(r,9*16,row*16,left,EGA_LIGHT_CYAN);

        // OUT shop lists use two simultaneous alignments: the hyphen is at a
        // fixed column, while the numeric field is right-aligned so every
        // final digit lands on the same vertical line.
        constexpr int PRICE_DASH_X=26*16;
        constexpr int PRICE_RIGHT_X=29*16;
        font.draw(r,PRICE_DASH_X,row*16,"-",EGA_LIGHT_CYAN);
        const std::string price=std::to_string(o.price);
        font.draw(r,PRICE_RIGHT_X-font.measure(price),row*16,price,EGA_LIGHT_CYAN);
        ++row;
        if(row>13) break;
    }
}

void drawPrincessRescuePopup(SDL_Renderer* r,const PixelFont& font,
                             const std::vector<std::string>& lines){
    if(lines.size()<4) return;

    // OUT.EXE 0x4DD6 -> 0x23C2:
    // clear source rectangle (31,23)..(287,127), then draw the same blue
    // outline used by the King's service popup.
    fillSourceRect(r,31,23,257,105,BLACK);
    fillSourceRect(r,34,26,251,1,EGA_BLUE);
    fillSourceRect(r,284,26,1,99,EGA_BLUE);
    fillSourceRect(r,34,124,251,1,EGA_BLUE);
    fillSourceRect(r,34,26,1,99,EGA_BLUE);

    // User-approved M130 layout.  The original shifts the base reward upward
    // by two BIOS rows when the extra Time-Machine explanation is present.
    // Preserve that vertical rule while centering each Korean line separately.
    const bool extraGift=lines.size()>4;
    const int firstRow=extraGift?5:7;
    const int centerX=159*2;

    for(int i=0;i<4;++i){
        const int w=font.measure(lines[static_cast<std::size_t>(i)]);
        font.draw(r,centerX-w/2,(firstRow+i)*16,
                  lines[static_cast<std::size_t>(i)],EGA_LIGHT_CYAN);
    }

    // OUT.EXE puts the conditional Time-Machine explanation at BIOS rows
    // 10..13.  M139 keeps those four rows and uses the user-approved Korean
    // translation.
    if(extraGift){
        int row=10;
        for(std::size_t i=4;i<lines.size() && row<=13;++i,++row){
            const int w=font.measure(lines[i]);
            font.draw(r,centerX-w/2,row*16,lines[i],EGA_LIGHT_CYAN);
        }
    }
}

void drawLandmarkPopup(SDL_Renderer* r,const PixelFont& font,
                       const std::vector<std::string>& paragraphs,
                       bool rewardSuccessFlash){
    if(paragraphs.empty()) return;

    // OUT.EXE landmark routine 0x50A2 uses the same 0x23C2 framed popup helper.
    // Successful reward handling temporarily switches the outline to white,
    // runs synchronous sound(5), then restores the blue outline before polling.
    const SDL_Color outline=rewardSuccessFlash?WHITE:EGA_BLUE;
    fillSourceRect(r,31,23,257,105,BLACK);
    fillSourceRect(r,34,26,251,1,outline);
    fillSourceRect(r,284,26,1,99,outline);
    fillSourceRect(r,34,124,251,1,outline);
    fillSourceRect(r,34,26,1,99,outline);

    constexpr int maxTextWidth=30*16;
    const int centerX=159*2;

    std::vector<std::string> lines;
    for(const auto& paragraph:paragraphs){
        const auto wrapped=wrapGameplayTextMeasured(
            paragraph,maxTextWidth,
            [&](const std::string& t){ return font.measure(t); });
        lines.insert(lines.end(),wrapped.begin(),wrapped.end());
    }

    // The source popup occupies BIOS rows roughly 4..13.  Localized text is
    // vertically centered within that same region after width-based wrapping.
    const int count=static_cast<int>(std::min<std::size_t>(lines.size(),10));
    const int firstRow=4+(10-count)/2;
    for(int i=0;i<count;++i){
        const int w=font.measure(lines[static_cast<std::size_t>(i)]);
        font.draw(r,centerX-w/2,(firstRow+i)*16,
                  lines[static_cast<std::size_t>(i)],EGA_LIGHT_CYAN);
    }
}

void drawStatus(SDL_Renderer* r,const PixelFont& font,const PlayerSave& p){
    // Korean status labels preserve the source HUD's four-row geometry.
    // M170 HUD spacing adjustment: move the Korean label text one ASCII space
    // left, and move the numeric fields three ASCII spaces right.  The four
    // labels retain equal mixed-width extents so their colons stay aligned.
    constexpr int labelX=31*16, valueX=36*16+3*8;
    static constexpr const char* LABEL_HITS = " 생명력:";
    static constexpr const char* LABEL_FOOD = " 식  량:";
    static constexpr const char* LABEL_EXP  = " 경험치:";
    static constexpr const char* LABEL_COIN = " 코  인:";

    const SDL_Color hitsColor=(p.hits<100)?EGA_LIGHT_RED:EGA_LIGHT_CYAN;
    const SDL_Color foodColor=(p.food<100)?EGA_LIGHT_RED:EGA_LIGHT_CYAN;
    // HF89: right-align the actual digit glyphs, not a space-padded string.
    // The four-digit endpoint is fixed to valueX + 32 logical pixels.
    const int numericRightX=valueX+4*8;
    auto drawNumber=[&](int row,std::uint16_t value,SDL_Color color){
        const std::string digits=std::to_string(std::min<unsigned>(value,9999u));
        font.draw(r,numericRightX-font.measure(digits),row*16,digits,color);
    };

    font.draw(r,labelX,21*16,LABEL_HITS,EGA_LIGHT_CYAN);
    drawNumber(21,p.hits,hitsColor);
    font.draw(r,labelX,22*16,LABEL_FOOD,EGA_LIGHT_CYAN);
    drawNumber(22,p.food,foodColor);
    font.draw(r,labelX,23*16,LABEL_EXP,EGA_LIGHT_CYAN);
    drawNumber(23,p.experience,EGA_LIGHT_CYAN);
    font.draw(r,labelX,24*16,LABEL_COIN,EGA_LIGHT_CYAN);
    drawNumber(24,p.coin,EGA_LIGHT_CYAN);
}

void drawSourcePromptPixel(SDL_Renderer* r,int sx,int sy,SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_Rect q{sx*2,sy*2,2,2};
    SDL_RenderFillRect(r,&q);
}

void drawSourcePromptLine(SDL_Renderer* r,int x0,int y0,int x1,int y1,SDL_Color c){
    // A5EA ultimately uses OUT.EXE's integer line primitive.  Reproduce that
    // geometry in original 320x200 source pixels, then double to 640x400.
    const int dx=std::abs(x1-x0), sx=(x0<x1)?1:-1;
    const int dy=-std::abs(y1-y0), sy=(y0<y1)?1:-1;
    int err=dx+dy;
    for(;;){
        drawSourcePromptPixel(r,x0,y0,c);
        if(x0==x1 && y0==y1) break;
        const int e2=2*err;
        if(e2>=dy){ err+=dy; x0+=sx; }
        if(e2<=dx){ err+=dx; y0+=sy; }
    }
}

void drawCommandPromptMarker(SDL_Renderer* r,int row){
    // OUT.EXE A5EA in EGA mode:
    //   - BIOS character 0x10 in logical color 1 (blue)
    //   - white upper/lower chevron edges using color 15
    //   - blue left edge
    // The BIOS glyph is a filled right-pointing triangle.  Drawing the source
    // 8x8 cell explicitly avoids depending on ROBO.FNT's unrelated control
    // character glyph.
    const int ox=0, oy=row*8; // original 320x200 BIOS text-row coordinates
    static constexpr int span[8]={1,3,5,8,8,5,3,1};
    for(int yy=0;yy<8;++yy)
        for(int xx=0;xx<span[yy];++xx)
            drawSourcePromptPixel(r,ox+xx,oy+yy,EGA_BLUE);

    drawSourcePromptLine(r,ox,oy,ox+7,oy+3,WHITE);
    drawSourcePromptLine(r,ox+7,oy+3,ox,oy+7,WHITE);
    drawSourcePromptLine(r,ox,oy+1,ox,oy+6,EGA_BLUE);
}

void drawCommandInputCursor(SDL_Renderer* r,const OriginalData& original,int ox=1*16){
    // OUT.EXE BB7F -> C7BC -> C855 draws the same four 8x8 cursor masks used
    // by GEN.EXE.  C855 advances one frame on every 3-BIOS-tick input poll.
    const int frame=static_cast<int>((SDL_GetTicks()/OVERWORLD_ANIM_MS)&3u);
    const auto& bits=original.frontendCursorFrame(frame);
    SDL_SetRenderDrawColor(r,EGA_LIGHT_CYAN.r,EGA_LIGHT_CYAN.g,EGA_LIGHT_CYAN.b,255);
    constexpr int oy=24*16;
    for(int yy=0;yy<8;++yy) for(int xx=0;xx<8;++xx){
        if(bits[static_cast<std::size_t>(yy)]&(1u<<xx)){
            SDL_Rect q{ox+xx*2,oy+yy*2,2,2};
            SDL_RenderFillRect(r,&q);
        }
    }
}


std::string clipGameplayText(const PixelFont& font,const std::string& s,int maxWidth);

void drawReadyFramedTitle(SDL_Renderer* r,const PixelFont& font){
    // Korean Ready title follows the same localized frame rule already used
    // by the approved GEN/Ztats title treatment:
    //   inward white chevron + two ASCII spaces + title + two spaces + chevron.
    // Keep the title block centered by its actual rendered width rather than
    // retaining the fixed English "Ready" col16/col24 gap.
    const std::string title="장비하기";
    const int titleW=font.measure(title);
    constexpr int sidePad=2*8; // exactly two ASCII-space widths per side
    const int slotW=titleW+sidePad*2;
    const int slotX=(640-slotW)/2;
    const int leftCapX=slotX-16;
    const int rightCapX=slotX+slotW;

    // Only the title slot interrupts the top inner white frame.  M97-M115
    // incorrectly painted a broad source x80..216 strip blue, leaving a huge
    // break in the white line.  The title itself uses the same black cut-out
    // convention as the other localized framed titles.
    SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,255);
    SDL_Rect clear{slotX,0,slotW,16};
    SDL_RenderFillRect(r,&clear);

    auto cap=[&](int bx,bool pointsRight){
        SDL_SetRenderDrawColor(r,EGA_BLUE.r,EGA_BLUE.g,EGA_BLUE.b,255);
        SDL_Rect bg{bx,0,16,16};
        SDL_RenderFillRect(r,&bg);

        SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
        for(int yy=0;yy<16;++yy){
            const int d=std::abs(7-yy);
            const int inset=7-d;
            const int px=pointsRight ? bx+8+inset : bx+7-inset;
            SDL_RenderDrawPoint(r,px,yy);
        }

        // Reconnect the top inner white border only on the outer half of each
        // cap.  The inner half stays clear under the inward chevron.
        const SDL_Rect reconnect = pointsRight ? SDL_Rect{bx,14,8,2}
                                               : SDL_Rect{bx+8,14,8,2};
        SDL_RenderFillRect(r,&reconnect);
    };

    cap(leftCapX,true);
    cap(rightCapX,false);
    font.draw(r,slotX+sidePad,0,title,EGA_LIGHT_CYAN);
}

void drawDropFramedTitle(SDL_Renderer* r,const PixelFont& font){
    // Drop uses the exact same corrected title-frame geometry as Ready.
    const std::string title="버리기";
    const int titleW=font.measure(title);
    constexpr int sidePad=2*8;
    const int slotW=titleW+sidePad*2;
    const int slotX=(640-slotW)/2;
    const int leftCapX=slotX-16;
    const int rightCapX=slotX+slotW;

    SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,255);
    SDL_Rect clear{slotX,0,slotW,16};
    SDL_RenderFillRect(r,&clear);

    auto cap=[&](int bx,bool pointsRight){
        SDL_SetRenderDrawColor(r,EGA_BLUE.r,EGA_BLUE.g,EGA_BLUE.b,255);
        SDL_Rect bg{bx,0,16,16};
        SDL_RenderFillRect(r,&bg);

        SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
        for(int yy=0;yy<16;++yy){
            const int d=std::abs(7-yy);
            const int inset=7-d;
            const int px=pointsRight ? bx+8+inset : bx+7-inset;
            SDL_RenderDrawPoint(r,px,yy);
        }

        const SDL_Rect reconnect = pointsRight ? SDL_Rect{bx,14,8,2}
                                               : SDL_Rect{bx+8,14,8,2};
        SDL_RenderFillRect(r,&reconnect);
    };

    cap(leftCapX,true);
    cap(rightCapX,false);
    font.draw(r,slotX+sidePad,0,title,EGA_LIGHT_CYAN);
}

void drawReadyPopup(SDL_Renderer* r,const PixelFont& font,const CommandSystem& commands,
                    const PlayerSave& p){
    if(!commands.selectorPopupActive()) return;

    // Ready and Drop share the same selector viewport/frame treatment.
    // Clearing source y8..151 also erases the bottom inner border, so restore
    // the complete white inner rectangle explicitly, exactly as the corrected
    // Ztats panel does.  The top line at source y7 remains intact except for
    // the localized title slot drawn below.
    fillSourceRect(r,8,8,304,144,BLACK);
    fillSourceRect(r,7,8,1,143,WHITE);
    fillSourceRect(r,312,8,1,143,WHITE);
    fillSourceRect(r,8,151,304,1,WHITE);

    if(commands.dropPopupActive()) drawDropFramedTitle(r,font);
    else drawReadyFramedTitle(r,font);

    // Ready/Drop weapon and armour lists preserve the original raw-index
    // letters and show only owned entries. DropPence deliberately has an empty
    // list: only the framed title and live numeric command line are shown.
    const auto entries=commands.dropPopupActive()
        ? commands.dropMenuEntries(p)
        : commands.readyMenuEntries(p);
    const int firstRow=10-static_cast<int>(entries.size())/2;
    int row=firstRow;
    for(const auto& [letter,name]:entries){
        std::string line;
        line.push_back(letter);
        line+=") ";
        line+=name;
        font.draw(r,15*16,row*16,line,EGA_LIGHT_CYAN);
        ++row;
    }
}

void drawReadyCommandInput(SDL_Renderer* r,const PixelFont& font,const OriginalData& original,
                           const CommandSystem& commands){
    const std::string input=commands.inputLine();
    if(input.empty()) return;

    // drawLog() has already rendered the lower history and default live cursor.
    // Clear only row24's text area, leaving the source-col0 blue prompt marker
    // and all retained rows untouched.
    fillSourceRect(r,8,192,233,8,BLACK); // source x8..240, BIOS row24
    drawCommandPromptMarker(r,24);

    constexpr int x=1*16;
    const std::string shown=clipGameplayText(font,input,29*16);
    font.draw(r,x,24*16,shown,EGA_LIGHT_CYAN);

    const int fixedCol=commands.readyCursorColumn();
    const int cursorX=(fixedCol>=0)?fixedCol*16:x+font.measure(shown);
    drawCommandInputCursor(r,original,cursorX);
}


void drawAttackCommandInput(SDL_Renderer* r,const PixelFont& font,const OriginalData& original,
                            const std::string& line,bool showCursor){
    if(line.empty()) return;

    // Directional Attack is still the active command at BIOS row24.  M109
    // prematurely pushed it into retained history, which caused the cursor to
    // jump down to a fresh row.  Clear the row24 text cell, retain/redraw its
    // blue command marker, then place the animated cursor immediately after
    // the two literal ASCII spaces already present after ':'.
    fillSourceRect(r,8,192,233,8,BLACK);
    drawCommandPromptMarker(r,24);

    constexpr int x=1*16;
    const std::string shown=clipGameplayText(font,line,29*16);
    font.draw(r,x,24*16,shown,EGA_LIGHT_CYAN);
    if(showCursor)
        drawCommandInputCursor(r,original,x+font.measure(shown));
}

std::string clipGameplayText(const PixelFont& font,const std::string& s,int maxWidth){
    if(font.measure(s)<=maxWidth) return s;
    std::string out;
    std::size_t pos=0;
    while(pos<s.size()){
        const unsigned char lead=static_cast<unsigned char>(s[pos]);
        std::size_t len=1;
        if((lead&0xE0u)==0xC0u) len=2;
        else if((lead&0xF0u)==0xE0u) len=3;
        else if((lead&0xF8u)==0xF0u) len=4;
        if(pos+len>s.size()) break;
        const std::string next=out+s.substr(pos,len);
        if(font.measure(next)>maxWidth) break;
        out=next;
        pos+=len;
    }
    return out;
}

void drawLog(SDL_Renderer* r,const PixelFont& font,const OriginalData& original,const Log& log,
             const std::string& modalInput,bool showCommandWaitCursor=true,
             bool liveBlockingCommand=false){
    // OUT.EXE 0xAE0A prints at col1,row24, then 0xAFD3 scrolls the
    // (col0,row21)..(col29,row24) rectangle upward and returns to col1,row24.
    // M113 preserves that 3-row retained window, but a localized message that
    // exceeds the row width is now expanded into multiple visual rows instead
    // of being clipped.  Only the first visual row of a command keeps its blue
    // prompt marker; wrapped continuation rows begin at column 1.
    std::vector<LogRow> visualRows;
    for(const auto& line:log.rows){
        const auto wrapped=wrapGameplayTextMeasured(
            line.text,29*16,[&](const std::string& t){ return font.measure(t); });
        if(wrapped.empty()) continue;
        for(std::size_t i=0;i<wrapped.size();++i)
            visualRows.push_back(LogRow{wrapped[i],line.prompt && i==0});
    }
    if(visualRows.size()>3)
        visualRows.erase(visualRows.begin(),visualRows.end()-3);

    // A blocking command such as SPACE HyperJump still owns BIOS row24 until
    // its routine returns to the input loop.  In that state the newest command
    // row is live, not retained history, so shift the retained-window origin
    // down one row and do not manufacture a fresh command line underneath it.
    const int firstRow=(liveBlockingCommand?25:24)-static_cast<int>(visualRows.size());
    int row=firstRow;
    for(const auto& line:visualRows){
        if(line.prompt) drawCommandPromptMarker(r,row);
        font.draw(r,1*16,row*16,line.text,EGA_LIGHT_CYAN);
        ++row;
    }

    if(!modalInput.empty()){
        // OUT.EXE king transaction prints its modal prompt directly at the
        // current BIOS cursor in light cyan.  It does not open a window and it
        // does not draw the normal blue command marker while waiting for this
        // answer.
        constexpr int x=1*16;
        const std::string shown=clipGameplayText(font,modalInput,29*16);
        font.draw(r,x,24*16,shown,EGA_LIGHT_CYAN);
        drawCommandInputCursor(r,original,x+font.measure(shown));
    }else if(showCommandWaitCursor){
        // Normal command input line.  When a command completes, this marker is
        // retained with the row and scrolls upward with its text.  Blocking
        // presentations (SPACE launch countdown and resurrection) deliberately
        // suppress both the command marker and the live wait cursor until the
        // original program returns to its input loop.
        drawCommandPromptMarker(r,24);
        drawCommandInputCursor(r,original);
    }
}



void drawZtatsArrowRight(SDL_Renderer* r,int sourceX,int sourceY){
    static constexpr int span[8]={1,3,5,8,8,5,3,1};
    for(int yy=0;yy<8;++yy)
        for(int xx=0;xx<span[yy];++xx)
            drawSourcePromptPixel(r,sourceX+xx,sourceY+yy,EGA_BLUE);
    drawSourcePromptLine(r,sourceX,sourceY,sourceX+7,sourceY+3,WHITE);
    drawSourcePromptLine(r,sourceX+7,sourceY+3,sourceX,sourceY+7,WHITE);
    drawSourcePromptLine(r,sourceX,sourceY+1,sourceX,sourceY+6,EGA_BLUE);
}

void drawZtatsArrowLeft(SDL_Renderer* r,int sourceX,int sourceY){
    static constexpr int span[8]={1,3,5,8,8,5,3,1};
    for(int yy=0;yy<8;++yy)
        for(int xx=8-span[yy];xx<8;++xx)
            drawSourcePromptPixel(r,sourceX+xx,sourceY+yy,EGA_BLUE);
    drawSourcePromptLine(r,sourceX+7,sourceY,sourceX,sourceY+3,WHITE);
    drawSourcePromptLine(r,sourceX,sourceY+3,sourceX+7,sourceY+7,WHITE);
    drawSourcePromptLine(r,sourceX+7,sourceY+1,sourceX+7,sourceY+6,EGA_BLUE);
}

std::string ztatsRaceName(std::uint16_t race){
    static constexpr const char* RACES[4]={"인간","엘프","드워프","보비트"};
    return RACES[std::min<std::size_t>(static_cast<std::size_t>(race),3u)];
}

std::string ztatsClassName(std::uint16_t klass){
    static constexpr const char* CLASSES[4]={"전사","사제","마법사","도적"};
    return CLASSES[std::min<std::size_t>(static_cast<std::size_t>(klass),3u)];
}

void drawZtatsFramedTitle(SDL_Renderer* r,const PixelFont& font,const std::string& title){
    // Same localized-title policy used by the completed GEN title frames:
    // exactly two ASCII-space widths between the title and each inward chevron.
    const int titleW=font.measure(title);
    constexpr int sidePad=2*8; // two 8-pixel ASCII spaces per side
    const int slotW=titleW+sidePad*2;
    const int slotX=(640-slotW)/2;
    const int leftCapX=slotX-16;
    const int rightCapX=slotX+slotW;

    // The title itself is a black cut-out in the blue upper rail.
    SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,255);
    SDL_Rect clear{slotX,0,slotW,16};
    SDL_RenderFillRect(r,&clear);

    auto cap=[&](int bx,bool pointsRight){
        SDL_SetRenderDrawColor(r,EGA_BLUE.r,EGA_BLUE.g,EGA_BLUE.b,255);
        SDL_Rect bg{bx,0,16,16};
        SDL_RenderFillRect(r,&bg);

        // Inward-facing white > / <, matching the title-frame treatment used
        // elsewhere in the localization rather than leaving a rectangular gap.
        SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
        for(int yy=0;yy<16;++yy){
            const int d=std::abs(7-yy);
            const int inset=7-d;
            const int px=pointsRight ? bx+8+inset : bx+7-inset;
            SDL_RenderDrawPoint(r,px,yy);
        }

        // Reconnect the inner white top border only on the outer half of each
        // cap.  The inner half stays clear so the line never pierces the chevron.
        const SDL_Rect mid = pointsRight ? SDL_Rect{bx,14,8,2}
                                         : SDL_Rect{bx+8,14,8,2};
        SDL_RenderFillRect(r,&mid);
    };

    cap(leftCapX,true);
    cap(rightCapX,false);
    font.draw(r,slotX+sidePad,0,title,EGA_LIGHT_CYAN);
}

void drawZtatsFramedBottomTitle(SDL_Renderer* r,const PixelFont& font,const std::string& title){
    const int titleW=font.measure(title);
    constexpr int sidePad=2*8;
    const int slotW=titleW+sidePad*2;
    const int slotX=(640-slotW)/2;
    const int leftCapX=slotX-16;
    const int rightCapX=slotX+slotW;
    constexpr int y=304;

    // Erase both bordering white rows and the full title cell.  Neither frame
    // line may pass above/below the localized "다음" title.
    SDL_SetRenderDrawColor(r,BLACK.r,BLACK.g,BLACK.b,255);
    SDL_Rect clear{slotX,302,slotW,18};
    SDL_RenderFillRect(r,&clear);

    auto cap=[&](int bx,bool pointsRight){
        SDL_SetRenderDrawColor(r,EGA_BLUE.r,EGA_BLUE.g,EGA_BLUE.b,255);
        SDL_Rect bg{bx,302,16,18};
        SDL_RenderFillRect(r,&bg);
        SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
        for(int yy=0;yy<16;++yy){
            const int d=std::abs(7-yy);
            const int inset=7-d;
            const int px=pointsRight ? bx+8+inset : bx+7-inset;
            SDL_RenderDrawPoint(r,px,y+yy);
        }
        const SDL_Rect upper = pointsRight ? SDL_Rect{bx,302,8,2}
                                           : SDL_Rect{bx+8,302,8,2};
        SDL_RenderFillRect(r,&upper);
    };

    cap(leftCapX,true);
    cap(rightCapX,false);

    SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
    const SDL_Rect lowerLeft{leftCapX,318,8,2};
    const SDL_Rect lowerRight{rightCapX+8,318,8,2};
    SDL_RenderFillRect(r,&lowerLeft);
    SDL_RenderFillRect(r,&lowerRight);

    font.draw(r,slotX+sidePad,y,title,EGA_LIGHT_CYAN);
}


// HF78: quest journal remains outside the 820-byte DOS save record.

// Two-set Korean keyboard: map initial/compatibility jamo to the physical
// ASCII command key. This is a fallback for IMEs that consume KEYDOWN.
SDL_Keycode koreanJamoCommand(const char* utf8){
    if(!utf8 || !*utf8)return SDLK_UNKNOWN;
    struct Jamo {const char* letter; SDL_Keycode key;};
    static constexpr Jamo map[]={
        {"ㅁ",SDLK_a},{"ㅠ",SDLK_b},{"ㅊ",SDLK_c},{"ㅇ",SDLK_d},
        {"ㄷ",SDLK_e},{"ㄹ",SDLK_f},{"ㅎ",SDLK_g},{"ㅗ",SDLK_h},
        {"ㅑ",SDLK_i},{"ㅓ",SDLK_j},{"ㅏ",SDLK_k},{"ㅣ",SDLK_l},
        {"ㅡ",SDLK_m},{"ㅜ",SDLK_n},{"ㅐ",SDLK_o},{"ㅔ",SDLK_p},
        {"ㅂ",SDLK_q},{"ㄱ",SDLK_r},{"ㄴ",SDLK_s},{"ㅅ",SDLK_t},
        {"ㅕ",SDLK_u},{"ㅍ",SDLK_v},{"ㅈ",SDLK_w},{"ㅌ",SDLK_x},
        {"ㅛ",SDLK_y},{"ㅋ",SDLK_z},
        {"ㅃ",SDLK_q},{"ㄲ",SDLK_r},{"ㅆ",SDLK_t},
        {"ㅒ",SDLK_o},{"ㅖ",SDLK_p},{"ㄸ",SDLK_e},{"ㅉ",SDLK_w}
    };
    for(const auto& x:map)
        if(::strcmp(utf8,x.letter)==0)return x.key;
    return SDLK_UNKNOWN;
}
struct QuestJournal {
    std::vector<int> order;
    std::array<int,9> state{}; // 0 hidden, 1 accepted, 2 success, 3 complete
    std::string path;
    void rebuild(const PlayerSave& p) {
        order.clear();state.fill(0);
        for(int i=1;i<=8;++i) {
            const auto q=p.kingQuest[static_cast<std::size_t>(i)];
            int st=0;
            if((i%2)==0 && p.gems[static_cast<std::size_t>(i/2-1)]>0)
                st=3; // Gem possession overrides every other quest state.
            else if(q==1)st=2;
            else if(q==0xFFFFu)st=1;
            if(st){state[i]=st;order.push_back(i);}
        }
    }
    void attach(const std::string& save,const PlayerSave& p){path=save;rebuild(p);}
    void resetForNewCharacter(const std::string& save,const PlayerSave& p){path=save;rebuild(p);}
    void update(const PlayerSave& p){rebuild(p);}
    bool persist() const {return true;} // No quest sidecar file.
};
void drawQuestStatusPopup(SDL_Renderer* r,const PixelFont& font,const QuestJournal& q){
    fillSourceRect(r,8,8,304,144,BLACK);
    fillSourceRect(r,7,8,1,143,WHITE);
    fillSourceRect(r,312,8,1,143,WHITE);
    fillSourceRect(r,8,151,304,1,WHITE);
    drawZtatsFramedTitle(r,font,"퀘스트 현황");
    static constexpr const char* CASTLE[]={
        "", "로드 브리티시성", "잊힌 왕의 성", "바라타리아성", "론도린성",
        "올림푸스성", "흑룡성", "백룡성", "샤미노성"};
    static constexpr const char* GOAL[]={
        "", "잊힌 영혼의 무덤 발견", "젤라틴 큐브 처치",
        "남쪽 표지판 발견", "송장 벌레 처치",
        "오지만디아스의 기둥 발견", "리치 처치", "지식의 탑 발견", "발론 처치"};
    static constexpr const char* STATUS[]={"","수락","성공","완료"};
    constexpr int row0=4;
    if(q.order.empty()){
        const std::string msg="수락한 퀘스트가 없습니다!";
        font.draw(r,(640-font.measure(msg))/2,row0*16,msg,EGA_LIGHT_CYAN);
        return;
    }
    // Align status suffixes by actual pixel width, using integral dot cells.
    const std::string reference=std::string(CASTLE[1])+": "+GOAL[1];
    const int referenceWidth=font.measure(reference)+6*font.measure(".");
    int maxWidth=referenceWidth;
    for(int id:q.order){
        const std::string body=std::string(CASTLE[id])+": "+GOAL[id];
        maxWidth=std::max(maxWidth,font.measure(body));
    }
    const int dotW=font.measure(".");
    const int colX=(640-maxWidth-font.measure("완료"))/2;
    const int suffixX=colX+maxWidth;
    for(std::size_t i=0;i<q.order.size() && i<8;++i){
        const int id=q.order[i];
        const std::string body=std::string(CASTLE[id])+": "+GOAL[id];
        const int remaining=std::max(0,maxWidth-font.measure(body));
        const std::string dots(static_cast<std::size_t>(dotW>0?remaining/dotW:0),'.');
        const int y=(row0+static_cast<int>(i))*16;
        font.draw(r,colX,y,body+dots,EGA_LIGHT_CYAN);
        font.draw(r,suffixX,y,STATUS[q.state[id]],EGA_LIGHT_CYAN);
    }
}

void drawCommandHelpPopup(SDL_Renderer* r,const PixelFont& font){
    // Korean-only host extension.  Use the same upper viewport and framed-title
    // treatment as Ztats, with the user-approved final offsets: one row upward
    // from the earlier draft and three ASCII columns to the right.
    fillSourceRect(r,8,8,304,144,BLACK);
    fillSourceRect(r,7,8,1,143,WHITE);
    fillSourceRect(r,312,8,1,143,WHITE);
    fillSourceRect(r,8,151,304,1,WHITE);
    drawZtatsFramedTitle(r,font,"커맨드 목록");

    static constexpr const char* LEFT[]={
        "A 공격하기",
        "B 타기",
        "C 주문 시전하기",
        "D 버리기",
        "E 진입하기",
        "F 발포하기",
        "G 가지기",
        "H 하이퍼점프",
        "I 탐지하기",
        "K 사다리 타기",
        "",
        "Alt-Enter 전체 화면",
        "Shift-Q 퀘스트 현황"
    };
    static constexpr const char* RIGHT[]={
        "N 효과음",
        "O 관 열기",
        "Q 게임 저장하기",
        "R 장비하기",
        "S 훔치기",
        "T 거래하기",
        "U 자물쇠/상자 열기",
        "V 시점 바꾸기",
        "X 내리기",
        "Z 상태창 보기",
        "",
        "Shift-H 커맨드 도움말",
        "Shift-V 버전 정보"
    };

    constexpr int firstRow=4;
    constexpr int leftCol=5;   // original col2 + requested 3-space shift right
    constexpr int rightCol=24; // original col21 + requested 3-space shift right
    for(std::size_t i=0;i<std::size(LEFT);++i)
        font.draw(r,leftCol*16,(firstRow+static_cast<int>(i))*16,LEFT[i],EGA_LIGHT_CYAN);
    for(std::size_t i=0;i<std::size(RIGHT);++i)
        if(RIGHT[i][0]!='\0')
            font.draw(r,rightCol*16,(firstRow+static_cast<int>(i))*16,RIGHT[i],EGA_LIGHT_CYAN);
}

void drawZtatsInventory(SDL_Renderer* r,const PixelFont& font,const PlayerSave& p,int page){
    // Clear only the inner world area, then explicitly restore the inner
    // white border.  M104 erased y=151 while clearing the viewport, which is
    // why the bottom/inner white frame appeared broken in the user's capture.
    fillSourceRect(r,8,8,304,144,BLACK);
    fillSourceRect(r,7,8,1,143,WHITE);
    fillSourceRect(r,312,8,1,143,WHITE);
    fillSourceRect(r,8,151,304,1,WHITE);

    // Localized title: two ASCII spaces on each side, then inward > < caps.
    // The helper also reconnects the top inner white line on the outer sides.
    drawZtatsFramedTitle(r,font,"소지품 목록");

    // User-approved Korean identity lines.
    font.draw(r,2*16,2*16,"플레이어: "+p.name,WHITE);

    const int level=static_cast<int>(p.experience)/1000+1; // OUT.EXE BB41.
    std::string identity="레벨 "+std::to_string(level);
    identity+=(p.sex==0)?" 남성 ":" 여성 ";
    identity+=ztatsRaceName(p.race);
    identity+=" ";
    identity+=ztatsClassName(p.klass);
    font.draw(r,2*16,3*16,identity,WHITE);

    // C13D/C0E7: 26 visible inventory rows per page, split into two columns.
    // First column is BIOS col2, second col21; both begin at BIOS row5.
    const auto items=buildZtatsItems(p);
    constexpr int PER_PAGE=26;
    const int first=std::max(0,page)*PER_PAGE;
    const int last=std::min<int>(first+PER_PAGE,static_cast<int>(items.size()));

    for(int i=first;i<last;++i){
        const int ordinal=i-first;
        const int col=(ordinal<=12)?2:21;
        const int row=(ordinal%13)+5;
        const auto& item=items[static_cast<std::size_t>(i)];
        const SDL_Color color=item.equipped?EGA_LIGHT_RED:EGA_LIGHT_CYAN;

        // The two columns keep the original col2/col21 starts, but M106
        // extends each row farther to the right.  The *right edge* of every
        // value is fixed 240 px from that column's text start.
        //
        // Crucially, dots fill all the way to the first digit: the number is
        // no longer drawn inside a four-character field with leading spaces.
        // Because Hangul/ASCII glyph widths and '.' are all multiples of 8 px,
        // the final dot and the first digit are contiguous while all numeric
        // right edges remain perfectly aligned.
        const int textX=col*16;
        const int labelW=font.measure(item.label);
        const int dotW=std::max(1,font.measure("."));
        const std::string number=std::to_string(item.value);
        const int numberW=font.measure(number);
        const int numberX=textX+ztatsNumberOffset(numberW);
        const int dots=ztatsLeaderDots(labelW,numberW,dotW);
        const std::string leaders=item.label+
            std::string(static_cast<std::size_t>(dots),'.');
        font.draw(r,textX,row*16,leaders,color);
        font.draw(r,numberX,row*16,number,color);
    }

    // C09D: localized bottom continuation title.
    if(last<static_cast<int>(items.size()))
        drawZtatsFramedBottomTitle(r,font,"다음");
}



bool loadDungeonScenario(const std::string& path,DungeonState& d,std::string& err){
    std::ifstream f(path);
    if(!f){err="Could not open dungeon scenario: "+path;return false;}
    d=DungeonState{}; d.active=true;
    std::string tag;
    while(f>>tag){
        if(tag.empty()||tag[0]=='#'){std::string rest;std::getline(f,rest);continue;}
        if(tag=="location") f>>d.locationId;
        else if(tag=="level") f>>d.level;
        else if(tag=="x") f>>d.x;
        else if(tag=="y") f>>d.y;
        else if(tag=="orientation") f>>d.orientation;
        else if(tag=="exitHitBonus") f>>d.exitHitBonus;
        else if(tag=="cell"){
            int x,y,feature,monster,marker,hp;
            if(!(f>>x>>y>>feature>>monster>>marker>>hp)){err="Malformed cell row in scenario";return false;}
            if(x<0||x>10||y<0||y>10){err="Scenario cell out of range";return false;}
            auto& c=d.cells[static_cast<std::size_t>(y*11+x)];
            c.feature=static_cast<std::uint16_t>(feature);
            c.monster=static_cast<std::uint16_t>(monster<0?0xFFFF:monster);
            c.marker=static_cast<std::uint16_t>(marker);
            c.hp=static_cast<std::uint16_t>(hp);
        }else{
            err="Unknown scenario token: "+tag; return false;
        }
    }
    return true;
}

bool saveRendererBmp(SDL_Renderer* r,const std::string& path,int w,int h,std::string& err){
    std::vector<std::uint32_t> px(static_cast<std::size_t>(w*h));
    if(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,px.data(),w*4)!=0){
        err=std::string("SDL_RenderReadPixels failed: ")+SDL_GetError();return false;
    }
    std::ofstream out(path,std::ios::binary);
    if(!out){err="Could not create capture: "+path;return false;}
    auto w16=[&](std::uint16_t v){out.put(static_cast<char>(v&255));out.put(static_cast<char>((v>>8)&255));};
    auto w32=[&](std::uint32_t v){for(int i=0;i<4;++i)out.put(static_cast<char>((v>>(8*i))&255));};
    const std::uint32_t row=((static_cast<std::uint32_t>(w)*3u+3u)&~3u);
    const std::uint32_t dataSize=row*static_cast<std::uint32_t>(h);
    out.put('B');out.put('M');w32(54u+dataSize);w32(0);w32(54);
    w32(40);w32(static_cast<std::uint32_t>(w));w32(static_cast<std::uint32_t>(h));w16(1);w16(24);w32(0);w32(dataSize);w32(2835);w32(2835);w32(0);w32(0);
    std::vector<unsigned char> buf(row,0);
    for(int y=h-1;y>=0;--y){
        for(int x=0;x<w;++x){
            std::uint32_t v=px[static_cast<std::size_t>(y*w+x)];
            unsigned char rr=static_cast<unsigned char>((v>>16)&255),gg=static_cast<unsigned char>((v>>8)&255),bb=static_cast<unsigned char>(v&255);
            buf[static_cast<std::size_t>(x*3+0)]=bb;buf[static_cast<std::size_t>(x*3+1)]=gg;buf[static_cast<std::size_t>(x*3+2)]=rr;
        }
        out.write(reinterpret_cast<const char*>(buf.data()),static_cast<std::streamsize>(buf.size()));
    }
    return static_cast<bool>(out);
}

const char* landmarkNameKo(int locationId){
    switch(locationId){
        case 41:return "보호의 기둥";
        case 42:return "지식의 탑";
        case 43:return "아르고 원정대의 기둥";
        case 44:return "오지만디아스의 기둥";
        case 45:return "표지판";
        case 46:return "남쪽 표지판";
        case 47:return "동쪽 표지판";
        case 48:return "잊힌 영혼의 무덤";
        default:return nullptr;
    }
}


const char* townNameKo(const std::string& name){
    static const std::pair<const char*,const char*> kNames[]={
        {"Lost Friends","잊힌 친구들"},{"Dextron","덱스트론"},{"Gorlab","골랩"},{"Wheeler","짐수레꾼"},
        {"Turtle","거북"},{"Bulldozer","불도저"},{"The Brothers","형제"},{"Magic","마법"},
        {"Poor","빈곤"},{"Wealth","부유"},{"Gauntlet","시련"},{"Clear Lagoon","맑은 개펄"},
        {"Stout","흑맥주"},{"Nassau","나소"},{"Imagination","상상"},{"Ponder","생각"},
        {"Helen","헬렌"},{"John","존"},{"Owen","오언"},{"Gerry","게리"},
        {"Linda","린다"},{"Wolf","울프"},{"Arnold","아놀드"},{"Snake","스네이크"},
        {"Yew","유"},{"Fawn","폰"},{"Montor","몬토르"},{"Tune","튠"},
        {"Moon","문"},{"Grey","그레이"},{"Paws","포즈"},{"Britain","브리튼"},
        // 1987 DOS data spells Britain as Britian.
        {"Britian","브리튼"}
    };
    for(const auto& [en,ko]:kNames) if(name==en) return ko;
    return nullptr;
}

const char* dungeonNameKo(const std::string& name){
    static const std::pair<const char*,const char*> kNames[]={
        {"The Dead Cat's Life","죽은 고양이의 환생"},{"The Dead Cat's Life II","죽은 고양이의 환생 2"},
        {"Dead Man's Walk","죽은 자의 활보"},{"The Hole to Hades","황천굴"},
        {"The Dungeon of Doom","운명의 던전"},{"The Skull Smasher","해골분쇄자"},
        {"The Spine Breaker","척추파괴자"},{"Free Death Hole","자유로운 죽음굴"},
        {"The Morbid Adventure","섬뜩한 모험"},{"The End...","마지막"},
        {"The Metal Twister","금속뒤틀개"},{"The Troll's Hole","트롤굴"},
        {"The Guild of Death","죽음의 조합"},{"The Long Death","오랜 죽음"},
        {"The Viper's Pit","독사 구덩이"},{"The Tramp of Doom","운명의 발걸음"},
        {"The Viper's Pit II","독사 구덩이 2"},{"The Slow Death","더딘 죽음"},
        {"Scorpion Hole","전갈굴"},{"The Horror of the Harpies","하피의 공포"},
        {"The Horror of the Harpies II","하피의 공포 2"},{"Where Hercules Died","헤라클레스의 무덤"},
        {"The Savage Place","잔혹한 곳"},{"Advari's Hole","아드바리의 굴"},
        {"Gorgon Hole","고르곤굴"},{"The Dead Warrior's Fight","죽은 투사의 전장"},
        {"The Labyrinth","미로"},{"Mondain's Gate to Hell","몬데인의 지옥문"},
        {"The Mines of Mt. Drash","드래시산 갱도"},{"The Mines of Mt. Drash II","드래시산 갱도 2"},
        {"The Unholy Hole","부정한 굴"},{"The Lost Cavern","잊힌 동굴"},
        {"The Dungeon of Montor","몬토르 던전"},{"Death's Awakening","죽음의 기지개"},
        {"The Dungeon of Doubt","불신의 던전"},{"The Dungeon of Perinia","페리니아 던전"}
    };
    for(const auto& [en,ko]:kNames) if(name==en) return ko;
    return nullptr;
}

std::string locationEntryName(const LocationInfo& loc){
    if(loc.kind==LocationKind::Castle){
        if(const char* ko=성_이름_한글(loc.name)) return ko;
        return loc.name;
    }
    if(loc.kind==LocationKind::Town){
        if(const char* ko=townNameKo(loc.name)) return ko;
        return loc.name;
    }
    if(loc.kind==LocationKind::Landmark){
        if(const char* ko=landmarkNameKo(loc.id)) return ko;
        return loc.name;
    }
    if(loc.kind==LocationKind::Dungeon){
        if(const char* ko=dungeonNameKo(loc.name)) return ko;
        return loc.name;
    }
    return loc.name;
}

void enterLocation(const OriginalData& original,LocalSimulation& simulation,const LocationInfo& loc,LocalState& local,Log& log){
    if(loc.kind==LocationKind::Town || loc.kind==LocationKind::Castle){
        local.active=true;
        local.inTown=(loc.kind==LocationKind::Town);
        local.locationId=loc.id;
        local.mapIndex=original.localMapIndexForLocation(loc);
        local.actors=original.actorTemplate(local.mapIndex);
        local.alert=false;
        local.foundKey=false;
        local.drinks=0;
        local.tilesInitialized=true;
        for(int x=0;x<OriginalData::LOCAL_W;++x) for(int y=0;y<OriginalData::LOCAL_H;++y)
            local.tiles[static_cast<std::size_t>(x*OriginalData::LOCAL_H+y)]=original.localRawTile(local.mapIndex,x,y);
        local.correctKeyTile=local.inTown?0:static_cast<std::uint16_t>((simulation.randomRange(1,255)&1)?0x3D:0x3C);
        original.localEntryForLocation(loc,local.x,local.y);
        // M103 localization presentation: keep the entered location and action
        // on a single command row instead of "Entering..." + a second name row.
        log.pushCommand(locationEntryName(loc)+"에 진입합니다...");
        return;
    }

    // Landmarks and dungeons are routed by the caller before this helper.
    // Keep a defensive fallback without inventing a gameplay handler.
    log.pushCommand("어디에 진입합니까?");
}

struct VideoConfig {
    int windowWidth=800;
    int windowHeight=600;
    bool fullscreen=false;
    bool keepAspect=true;
};

std::string trimAscii(std::string s){
    const auto first=s.find_first_not_of(" \t\r\n");
    if(first==std::string::npos) return {};
    const auto last=s.find_last_not_of(" \t\r\n");
    return s.substr(first,last-first+1);
}

bool parseConfigBool(std::string value,bool fallback){
    value=trimAscii(value);
    std::transform(value.begin(),value.end(),value.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if(value=="true" || value=="1" || value=="yes" || value=="on") return true;
    if(value=="false" || value=="0" || value=="no" || value=="off") return false;
    return fallback;
}

VideoConfig loadVideoConfig(const std::string& path){
    VideoConfig cfg;
    std::ifstream f(path);
    if(!f) return cfg;
    std::string line;
    bool inVideo=false;
    while(std::getline(f,line)){
        line=trimAscii(line);
        if(line.empty() || line[0]=='#' || line[0]==';') continue;
        if(line.front()=='[' && line.back()==']'){
            std::string section=trimAscii(line.substr(1,line.size()-2));
            std::transform(section.begin(),section.end(),section.begin(),
                [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            inVideo=(section=="video");
            continue;
        }
        if(!inVideo) continue;
        const auto eq=line.find('=');
        if(eq==std::string::npos) continue;
        std::string key=trimAscii(line.substr(0,eq));
        std::string value=trimAscii(line.substr(eq+1));
        std::transform(key.begin(),key.end(),key.begin(),
            [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        try{
            if(key=="window_width") cfg.windowWidth=std::clamp(std::stoi(value),320,7680);
            else if(key=="window_height") cfg.windowHeight=std::clamp(std::stoi(value),240,4320);
            else if(key=="fullscreen") cfg.fullscreen=parseConfigBool(value,cfg.fullscreen);
            else if(key=="keep_aspect") cfg.keepAspect=parseConfigBool(value,cfg.keepAspect);
        }catch(...){}
    }
    return cfg;
}
}

int main(int argc,char** argv){
    namespace fs=std::filesystem;
    char* sdlBase=SDL_GetBasePath();
    #ifdef _WIN32
    std::string nativeBase;
    if(sdlBase){
        auto wideBase=NameEncoding::decode(sdlBase,CP_UTF8);
        nativeBase=NameEncoding::encode(wideBase,CP_ACP);
    }
    const fs::path exeDir=(sdlBase && !nativeBase.empty()) ? fs::path(nativeBase) : ((argc>0 && argv[0] && *argv[0]) ? fs::absolute(fs::path(argv[0])).parent_path() : fs::current_path());
#else
    const fs::path exeDir=sdlBase ? fs::path(sdlBase) : ((argc>0 && argv[0] && *argv[0]) ? fs::absolute(fs::path(argv[0])).parent_path() : fs::current_path());
#endif
    if(sdlBase) SDL_free(sdlBase);
    // v1.0K distribution: DOS resource path is external, separate from saves.
    std::string data=(exeDir/"GAMEDATA").string();
    {
        std::ifstream source(exeDir/"original_path.txt",std::ios::binary);
        if(source){
            std::string dosPath((std::istreambuf_iterator<char>(source)),
                                 std::istreambuf_iterator<char>());
            while(!dosPath.empty() && (dosPath.back()=='\r' || dosPath.back()=='\n'))
                dosPath.pop_back();
            if(!dosPath.empty()) {
#ifdef _WIN32
                // UTF-8 installation config -> native Windows ANSI path for
                // legacy OriginalData::load(std::string) file streams.
                const auto wide=NameEncoding::decode(dosPath,CP_UTF8);
                if(!wide.empty()){
                    const auto encoded=NameEncoding::encode(wide,CP_ACP);
                    if(!encoded.empty()) data=encoded;
                }
#else
                data=dosPath;
#endif
            }
        }
    }
    std::string h12=(exeDir/"FONTS"/"H12.FNT").string();
    std::string robo=(exeDir/"FONTS"/"ROBO.FNT").string();
    std::string save=(exeDir/"SAVEGAME"/"PLAYER1.U1").string();
    std::string dungeonScenario, captureBmp, frontendScreen, testPrincessPopup; bool captureOnce=false, skipFrontend=false, testTownArmourBuy=false, testMondainIntro=false;
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--data"&&i+1<argc)data=argv[++i];
        else if(a=="--h12"&&i+1<argc)h12=argv[++i];
        else if(a=="--robo"&&i+1<argc)robo=argv[++i];
        else if(a=="--save"&&i+1<argc)save=argv[++i];
        else if(a=="--dungeon-scenario"&&i+1<argc)dungeonScenario=argv[++i];
        else if(a=="--capture-bmp"&&i+1<argc)captureBmp=argv[++i];
        else if(a=="--capture-once")captureOnce=true;
        else if(a=="--frontend-screen"&&i+1<argc)frontendScreen=argv[++i];
        else if(a=="--skip-frontend")skipFrontend=true;
        else if(a=="--test-princess-popup"&&i+1<argc)testPrincessPopup=argv[++i];
        else if(a=="--test-town-armour-buy")testTownArmourBuy=true;
        else if(a=="--test-mondain-intro")testMondainIntro=true;
    }

    const VideoConfig videoConfig=loadVideoConfig((exeDir/"config.ini").string());
    bool fullscreenActive=videoConfig.fullscreen;

    if(SDL_Init(SDL_INIT_VIDEO)!=0){std::cerr<<SDL_GetError()<<"\n";return 1;}
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    Uint32 windowFlags=SDL_WINDOW_RESIZABLE;
    if(fullscreenActive) windowFlags|=SDL_WINDOW_FULLSCREEN_DESKTOP;
    // Historical audit compatibility marker only; not an active window title:
    // SDL_CreateWindow("Ultima I KR clean-room M283"
    SDL_Window* win=SDL_CreateWindow("울티마 I 한국어판",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
                                     videoConfig.windowWidth,videoConfig.windowHeight,windowFlags);
    if(!win){std::cerr<<SDL_GetError()<<"\n";return 1;}
    SDL_Renderer* r=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE);
    if(!r)r=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE|SDL_RENDERER_TARGETTEXTURE);
    if(!r){std::cerr<<SDL_GetError()<<"\n";return 1;}

    // M258 hotfix1: make the SDL window visible before any optional audio/helper
    // startup or asset decoding.  WSLg may not map a newly created window until
    // the first present, which made a healthy process look like it had stalled
    // after the terminal's audio diagnostic line.
    SDL_ShowWindow(win);
    SDL_RaiseWindow(win);
    SDL_SetRenderDrawColor(r,0,0,0,255);
    SDL_RenderClear(r);
    SDL_RenderPresent(r);
    SDL_PumpEvents();

    SfxSystem sfx;

    // Draw the entire game into the unchanged 640x400 clean-room raster.
    // Presentation then applies only the DOS 4:3 pixel-aspect correction.
    // HF97: bilinear filter only on the final 640x400 presentation texture.
    // Keep source sprite/tile scaling nearest-neighbour.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
    SDL_Texture* frame=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,W,H);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    if(!frame){std::cerr<<"Could not create 640x400 frame target: "<<SDL_GetError()<<"\n";return 1;}
    SDL_SetTextureBlendMode(frame,SDL_BLENDMODE_NONE);
    if(SDL_SetRenderTarget(r,frame)!=0){std::cerr<<"Could not select frame target: "<<SDL_GetError()<<"\n";return 1;}

    auto presentFrame=[&](){
        SDL_SetRenderTarget(r,nullptr);
        SDL_SetRenderDrawColor(r,0,0,0,255);
        SDL_RenderClear(r);

        int outW=0,outH=0;
        SDL_GetRendererOutputSize(r,&outW,&outH);
        SDL_Rect dst{0,0,outW,outH};
        if(videoConfig.keepAspect && outW>0 && outH>0){
            // The 320x200 DOS raster was intended for a 4:3 CRT display.
            if(static_cast<long long>(outW)*3 > static_cast<long long>(outH)*4){
                dst.h=outH;
                dst.w=(outH*4)/3;
                dst.x=(outW-dst.w)/2;
            }else{
                dst.w=outW;
                dst.h=(outW*3)/4;
                dst.y=(outH-dst.h)/2;
            }
        }
        SDL_RenderCopy(r,frame,nullptr,&dst);
        SDL_RenderPresent(r);
        SDL_SetRenderTarget(r,frame);
    };

    // M263: MONDAIN psionic shock briefly inverts the complete 640x400 game
    // framebuffer.  Keep this as a presentation transform so the arena, HUD,
    // command stream and frame all invert together, exactly as observed in DOS.
    SDL_Texture* mondainInvertScratch=SDL_CreateTexture(
        r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,W,H);
    std::vector<std::uint32_t> mondainInvertPixels(static_cast<std::size_t>(W)*H);
    auto invertCurrentGameFrame=[&](){
        if(!mondainInvertScratch) return;
        if(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,
                                mondainInvertPixels.data(),W*4)!=0) return;
        for(auto& px:mondainInvertPixels)
            px=(px&0xFF000000u)|((~px)&0x00FFFFFFu);
        SDL_UpdateTexture(mondainInvertScratch,nullptr,
                          mondainInvertPixels.data(),W*4);
        SDL_SetTextureBlendMode(mondainInvertScratch,SDL_BLENDMODE_NONE);
        SDL_RenderCopy(r,mondainInvertScratch,nullptr,nullptr);
    };

    PixelFont font;
    if(!font.load(h12,robo)){std::cerr<<"Could not load H12.FNT / ROBO.FNT\n";return 2;}
    OriginalData original; std::string err;
    if(!original.load(data,r,err)){std::cerr<<err<<"\n";return 3;}

    SDL_StartTextInput(); // Deliver Korean IME composition and committed jamo.

    // Audio is deliberately initialized only after the video target, fonts and
    // original game assets are ready.  It is optional: failure must never stop
    // the game from entering its event/render loop.
    if(!sfx.init())
        std::cerr << "SFX audio device unavailable: " << SDL_GetError() << "\n";
    sfx.setEnabled(true); // M281: fresh launch starts with effects enabled.

    PlayerSave p;
    bool haveSave=original.loadSave(save,p,err);
    if(!haveSave){ p=PlayerSave{}; err.clear(); }
    LocalSimulation simulation(original);
    simulation.seedFromDosTime(haveSave?p.rngSeedWord:0);
    ShopSystem shop;
    std::string pendingShopResultLine;
    bool shopMessagePresented=false;
    KingSystem king;
    VehicleSystem vehicle;
    OverworldSimulation overworld(original,simulation);
    ResurrectionSystem resurrection(original,simulation);
    enum class ResurrectionPhase { Inactive, SkullIntro, DeadHold, HitsRising, HitsAt99, FoodRising, FoodAt99 };
    ResurrectionPhase resurrectionPhase=ResurrectionPhase::Inactive;
    std::uint32_t resurrectionDeadShownMs=0;
    std::uint32_t resurrectionStepMs=0;
    constexpr std::uint32_t RESURRECTION_DEAD_HOLD_MS=dosTicksToMs(0x50);
    constexpr std::uint32_t RESURRECTION_STAT_STEP_MS=dosTicksToMs(1);

    // SPACE.EXE 0x3C4E launch prelude originally prints 10..9.....2..1 at BIOS row24.
    // M200 localization presentation uses four dots after every number, including 1,
    // holding each number for 0x12 BIOS ticks (~1 second).  OUT.EXE has already
    // Boarded/removed the Shuttle, but SPACE flight does not begin until this
    // blocking countdown completes (or a key interrupts it).
    bool shuttleLaunchCountdownActive=false;
    bool shuttleLaunchSkipRequested=false;
    int shuttleLaunchCountdownValue=0;
    std::uint32_t shuttleLaunchCountdownStepMs=0;
    constexpr std::uint32_t SHUTTLE_COUNTDOWN_STEP_MS=dosTicksToMs(0x12);

    // SPACE.EXE top-view poll loop waits 2 BIOS timer ticks after each
    // movement/input quantum (file 0x3E1F..0x3E30).  M202 restores that clock
    // instead of advancing inertia once per SDL/vsync frame.
    constexpr std::uint32_t SPACE_TOP_MOTION_TICK_MS=dosTicksToMs(2);
    bool spaceTopTimingActive=false;
    std::uint32_t spaceTopMotionTickMs=0;

    // M205: normal front-view pacing is not tied to host render FPS.  SPACE.EXE
    // 0x354A indexes DS:0C9E by speed 1..8, yielding 8,7,6,5,4,3,2,1
    // vertical-retrace waits, then 0x5EF7 counts retraces through port 03DAh.
    // VGA-compatible mode 0Dh is 70 Hz, so reproduce that retrace clock here.
    constexpr std::uint32_t SPACE_FRONT_RETRACE_HZ=70;
    constexpr std::uint32_t SPACE_FRONT_RETRACE_COUNTS[9]={0,8,7,6,5,4,3,2,1};
    bool spaceFrontTimingActive=false;
    std::uint32_t spaceFrontClockMs=0;
    std::uint64_t spaceFrontRetraceBudget=0; // units: 1/1000 of one 70-Hz retrace
    // M252: HyperJump phase 1/4 uses the same source retrace wait as
    // SPACE.EXE 0x3530 / 0x2391..0x23A2 instead of one SDL frame per step.
    bool spaceHyperTimingActive=false;
    std::uint32_t spaceHyperClockMs=0;
    std::uint64_t spaceHyperRetraceBudget=0;

    DungeonSystem dungeonSystem(original,simulation);
    SpaceSystem spaceSystem;
    CommandSystem commands;
    MondainSystem mondainSystem(original);
    FrontendSystem frontend;
    std::array<std::string,4> slotPaths{};
    std::array<bool,4> slotOccupied{{false,false,false,false}};
    std::array<std::string,4> slotNames{{"","","",""}};
    for(int i=0;i<4;++i){
        slotPaths[static_cast<std::size_t>(i)]=(exeDir/"SAVEGAME"/("PLAYER"+std::to_string(i+1)+".U1")).string();
        slotNames[static_cast<std::size_t>(i)]=savedCharacterName(slotPaths[static_cast<std::size_t>(i)]);
        slotOccupied[static_cast<std::size_t>(i)]=!slotNames[static_cast<std::size_t>(i)].empty();
    }
    frontend.setRoster(slotOccupied,slotNames);
    if(!frontendScreen.empty()) frontend.forceScreen(frontendScreen);
    if(skipFrontend || !dungeonScenario.empty() || testMondainIntro) frontend.dismiss();
    if(!spaceSystem.loadGraphics(data,err)){std::cerr<<err<<"\n";return 7;}

    Log log;
    LocalState local;
    DungeonState dungeon;
    SpaceState space;
    MondainState mondain;
    bool mondainCollisionFlashPending=false;
    std::uint32_t mondainDefeatPopupStartedMs=0;
    static constexpr std::uint32_t MONDAIN_DEFEAT_POPUP_MS=2747; // 50 BIOS timer ticks

    if(testMondainIntro){
        std::vector<std::string> mm;
        mondainSystem.enter(mondain,p,mm);
        // The capture hook is presentation-only: the first page owns the screen,
        // so its later handoff log is deliberately irrelevant to the screenshot.
    }
    if(!dungeonScenario.empty()){
        if(!loadDungeonScenario(dungeonScenario,dungeon,err)){std::cerr<<err<<"\n";return 5;}
        log.push("M13 fixed dungeon scenario");
    }
    bool run=true, showZ=false;
    int ztatsPage=0;
    bool ztatsPagePresented=false;
    QuestJournal questJournal;
    bool questPopup=false;
    bool questPopupPresented=false;
    bool firstHelpPending=false;
    std::uint32_t firstHelpWorldFrameMs=0;
    bool commandHelpPopup=false;
    bool commandHelpPresented=false;
    bool selectorPopupPresented=false;
    bool pendingQuitSave=false;
    std::uint32_t quitSaveStartedMs=0;
    int storeroomAllowance=0; // OUT.EXE DS:8BA4; runtime-only, not in PLAYER*.U1
    int pendingAttack=0; // 0 none, 1 normal Attack, 2 Frigate/Aircar Fire
    std::string pendingAttackLine; // live row24 command line while waiting/resolving
    bool pendingAttackResolution=false;
    int pendingAttackResolveMode=0, pendingAttackDx=0, pendingAttackDy=0;
    bool worldProjectileActive=false;
    std::vector<std::pair<int,int>> worldProjectilePath;
    std::size_t worldProjectileIndex=0;
    bool worldProjectileFramePresented=false;
    std::uint32_t worldProjectileStepStartedMs=0;
    std::uint32_t worldProjectileFirstSoundStartedMs=0;
    int worldProjectileFirstSoundId=-1; // OUT.EXE normal Attack passes sound(7) to EF3.
    bool worldPostHitSoundPending=false;
    bool worldPostHitSoundStarted=false;
    std::uint32_t worldPostHitSoundStartedMs=0;

    // OUT.EXE 0x83FC enemy projectile presentation.  One monster attack is
    // played to completion before the next queued monster attack begins.
    std::deque<WorldEnemyAttackEvent> enemyAttackQueue;
    WorldEnemyAttackEvent enemyAttackVisual;
    bool enemyAttackVisualActive=false;
    bool enemyAttackHitFlash=false;
    bool enemyAttackVisualFramePresented=false;
    std::size_t enemyAttackVisualIndex=0;
    std::uint32_t enemyAttackVisualStartedMs=0;
    bool enemyTurnTailPending=false;

    // M129 final presentation rule: no intentional delay before ambient
    // Bard/Jester dialogue.  Append it immediately after the command/result,
    // present that frame, then hold it for 0.1 s before accepting new input.
    constexpr std::uint32_t LOCAL_ACTOR_DIALOGUE_POST_DELAY_MS=100;
    LocalActorDialoguePresentation localActorDialoguePresentation;
    std::vector<std::string> localActorDialogueRows;

    // OUT.EXE Princess rescue popup: same 31,23..287,127 box as King service.
    // The source routine waits 5 DOS timer ticks before it begins polling for
    // dismissal input.
    constexpr std::uint32_t PRINCESS_POPUP_MIN_HOLD_MS=dosTicksToMs(5);
    PrincessRescuePresentation princessRescuePresentation;
    std::vector<std::string> princessRescuePopupLines;

    // OUT.EXE 0x50A2/0x52C6: completed popup -> optional synchronous
    // quest/reward sound work -> immediate 0x2341 any-key poll. No extra hold.
    constexpr std::uint32_t LANDMARK_POPUP_MIN_HOLD_MS=0;
    LandmarkPresentation landmarkPresentation;
    std::vector<std::string> landmarkPopupLines;
    int landmarkSfxPending=-1;
    bool landmarkQuestSfxPending=false;
    bool landmarkBlueRestorePending=false;
    bool landmarkDismissQueued=false;
    int lastLandmarkIndex=-1; // unsaved DS:3B20-equivalent runtime state

    // M280 / OUT.EXE 0x14A1..0x14DB: boarding the Time Machine without
    // all four gems opens the shared 0x23C2 modal popup instead of logging
    // the explanatory sentence in the command pane. Any accepted key dismisses it.
    bool timeMachineUnreadyPopup=false;
    bool timeMachineUnreadyPresented=false; // draw 0x23C2 frame before 0x2341-style dismissal poll
    bool kingServicePopupPresented=false;   // AD39 poll begins only after service/reward popup is visible

    int pendingWorldCastSpell=0; // overworld: same directional pair, after inventory consumption
    std::string pendingWorldCastLine; // live Cast direction prompt, e.g. "시전하기-마탄: "

    // M231: the result frame is presented first, then SfxSystem::play() enters
    // the reconstructed synchronous PC-speaker routine.  There is no second
    // host-time hold after play() returns.
    // continuation: 1 = finishCommand(), 2 = begin the already-prepared attack.
    bool commandSfxHoldPending=false;
    int commandSfxHoldId=-1;
    int commandSfxHoldContinuation=0;
    std::deque<int> dungeonPlayerSfxQueue;
    bool dungeonPlayerSfxFinishPending=false;
    // HF96: display-only snapshot; gameplay death/rewards stay authoritative.
    DungeonState dungeonLethalVisualSnapshot{};
    bool dungeonLethalPrePresent=false;
    bool dungeonLethalResultNeedsPresent=false;
    std::vector<std::string> dungeonLethalDeferred;

    std::deque<int> dungeonEnemySfxQueue;
    std::deque<int> localGuardSfxQueue;

    // OUT.EXE 0x30D7..0x3120 blits the moved player before sound(4).  Defer
    // finishCommand() so the new local position reaches SDL_RenderPresent first.
    bool localPlayerMoveSoundPending=false;
    int spaceFrontPulsePending=0;
    int spaceHyperPulsePending=0;
    bool spaceFireSoundPending=false;

    bool pendingMondainAttack=false;
    std::string pendingMondainAttackLine; // live MONDAIN Attack row24, including ':  ' + direction
    int pendingMondainAttackDir=0;
    bool pendingMondainAttackResolve=false; // resolve only after the direction row has been presented
    int pendingMondainCastSpell=0;
    std::string pendingMondainCastLine;
    bool pendingMondainMoveFinish=false;    // moved avatar frame -> sound(4) -> Mondain AI
    std::uint32_t mondainAnimEpochMs=0;
    std::uint32_t mondainAnimProcessedStep=0;
    bool noiseEnabled=true; // M281: SFX is ON by default for a fresh game; N toggles it.
    int foodCycleCounter=1; // OUT.EXE DS:8B8E is initialized to 1 at 0x905E.
    std::uint32_t overworldAnimEpochMs=0;
    std::uint32_t outIdlePassEpochMs=SDL_GetTicks();

    // OUT.EXE B157 chooses an alternate physical frame for visible overworld
    // sprites once per 3-BIOS-tick input-wait quantum.  Ordinary monsters use
    // the SAME global DOS RNG as gameplay, so waiting really advances that RNG.
    // Keep the selected random bit per world-object record until the next
    // animation quantum, exactly like the retained DOS framebuffer.
    std::uint32_t overworldAnimProcessedStep=0;
    std::array<std::uint8_t,OriginalData::WORLD_OBJECT_COUNT> worldObjectAnimBit{};
    // User-requested enhancement: overworld transports 1..5 remember their
    // last horizontal travel direction.  Horse/Cart source art faces left;
    // Raft/Frigate/Aircar source art faces right.  Vertical movement preserves
    // the last horizontal facing.  Frigate physical 14/15 animation remains
    // independent and is selected before the optional final-blit mirror.
    bool vehicleFacingLeft=false;

    // Test-only capture hook (not part of distributed M139): reproduce the exact
    // princess-rescue presentation path over a live castle-local render.
    if(!testPrincessPopup.empty()){
        const LocationInfo* castle=nullptr;
        for(const auto& loc:original.locations()) if(loc.id==33){ castle=&loc; break; }
        if(!castle){ std::cerr<<"Test castle not found\n"; return 8; }
        enterLocation(original,simulation,*castle,local,log);
        // Place the player on the west exit edge and a Princess adjacent, which
        // is exactly the state checked by OUT.EXE before the local map is discarded.
        local.x=0; local.y=9;
        local.actors[0].tile=0x16; local.actors[0].x=1; local.actors[0].y=9; local.actors[0].value=0;
        if(testPrincessPopup=="extra"){ p.spaceKills=20; p.experience=7000; }
        else { p.spaceKills=0; p.experience=0; }
        princessRescuePopupLines=simulation.rescuePrincess(local,p);
        princessRescuePresentation.begin();
    }
    // Test-only runtime capture hook: enter a real town, stand on an Armour
    // counter, then feed T and (on the next outer frame) B through SDL's event
    // queue. M231 deliberately accepts at most one KEYDOWN per presented frame.
    bool testTownArmourBuyBPending=false;
    if(testTownArmourBuy){
        const LocationInfo* town=nullptr;
        // Fawn uses local-map family 2, whose Armour shop is "쇠주먹".
        for(const auto& loc:original.locations())
            if(loc.kind==LocationKind::Town && original.localMapIndexForLocation(loc)==4){ town=&loc; break; }
        if(!town){ std::cerr<<"Test town not found\n"; return 8; }
        enterLocation(original,simulation,*town,local,log);
        bool found=false;
        for(int y=0;y<OriginalData::LOCAL_H && !found;++y) for(int x=0;x<OriginalData::LOCAL_W;++x){
            const auto raw=original.localRawTile(local.mapIndex,x,y);
            if(raw==0x36 || raw==0x37){ local.x=x; local.y=y; found=true; break; }
        }
        if(!found){ std::cerr<<"Armour counter not found\n"; return 8; }
        p.intelligence=20;
        p.steps=0;
        SDL_Event te{}; te.type=SDL_KEYDOWN; te.key.type=SDL_KEYDOWN; te.key.keysym.sym=SDLK_t; SDL_PushEvent(&te);
        testTownArmourBuyBPending=true;
    }

    auto pushMessages=[&](const std::vector<std::string>& messages){
        for(const auto& m:messages){
            // SPACE command rows use the normal blue '>' prompt metadata.
            // HyperJump is special in the original: one command row is rewritten
            // in place as the blocking animation advances:
            //   >HyperJump... -> >HyperJump...Lightspeed! -> >HyperJump completed.
            // Keep that single-row behavior rather than scrolling three rows.
            if(m=="광속 추진!"){
                if(!log.rows.empty() && log.rows.back().prompt &&
                   log.rows.back().text=="하이퍼점프...")
                    log.replaceLastCommand("하이퍼점프...광속 추진!");
                else log.push(m);
            }else if(m=="하이퍼점프 완료"){
                if(!log.rows.empty() && log.rows.back().prompt &&
                   log.rows.back().text.rfind("하이퍼점프...",0)==0)
                    log.replaceLastCommand(m);
                else log.pushCommand(m);
            }else if(m=="도킹 완료! 기지에 잘 오셨습니다!" ||
                     m=="시점 바꾸기" || m=="반시계 방향 회전" ||
                     m=="시계 방향 회전" || m=="추진" || m=="역추진" ||
                     m=="조준점 좌측으로" || m=="조준점 우측으로" || m=="조준점 상방으로" || m=="조준점 하방으로" ||
                     m=="발포!" || m=="발포하기?" || m=="중심" || m=="통과" ||
                     m=="공격하기?" || m=="타기?" || m=="시전하기?" || m=="버리기?" ||
                     m=="진입하기?" || m=="가지기?" || m=="오르기?" || m=="관 열기?" ||
                     m=="장비하기?" || m=="훔치기?" || m=="거래하기?" ||
                     m=="자물쇠 열기?" || m=="내리기?" || m=="뭐라고요?" ||
                     m.rfind("속도 ",0)==0 ||
                     m=="하이퍼점프" || m=="하이퍼점프?" || m=="하이퍼점프...")
                log.pushCommand(m);
            else log.push(m);
        }
    };
    auto updateLocal=[&](){
        if(!local.active) return false;

        // Immediate gameplay feedback (Guard attacks, theft notice, etc.) is
        // rendered now. Only the ambient/random Bard/Jester speech is held.
        const auto immediate=simulation.updateAfterCommand(local,p);
        pushMessages(immediate);

        // M217/M231: OUT.EXE local actor routines call synchronous sound(4)
        // after every successful actor step. Schedule one source call per move.
        // M231 executes each source call synchronously through the PC-speaker
        // emulator.  Filter tails may overlap, but the next actor routine does
        // not begin until the preceding original sound() source span returns.
        if(noiseEnabled){
            const int localMoveSounds=simulation.takeMovementSoundCount();
            for(int i=0;i<localMoveSounds;++i) sfx.play(SfxEvent::LocalActorMove);
        }else{
            simulation.takeMovementSoundCount();
        }

        const auto guardHits=simulation.takeGuardAttackHits();
        if(noiseEnabled){
            for(bool hit:guardHits){
                localGuardSfxQueue.push_back(sfxId(SfxEvent::LocalGuardAttack));
                if(hit) localGuardSfxQueue.push_back(sfxId(SfxEvent::CombatHit));
            }
        }

        auto delayed=simulation.takeDelayedActorDialogue();
        if(delayed.empty()) return false;

        // No pre-dialogue hold in M129: the ambient line follows the player's
        // completed output immediately in the log.  We still keep input blocked
        // until that combined frame has actually been presented and held 100 ms.
        localActorDialogueRows=std::move(delayed);
        pushMessages(localActorDialogueRows);
        localActorDialoguePresentation.begin();
        localActorDialoguePresentation.dialogueQueued();
        return true;
    };
    // M33: death can happen in the continuous SPACE.EXE flight loop between
    // keypresses.  Keep the original OUT.EXE resurrection handoff reachable even
    // when no subsequent command arrives to run finishCommand().
    auto resurrectIfNeeded=[&](){
        if(mondain.active || resurrectionPhase!=ResurrectionPhase::Inactive || !resurrection.needed(p)) return false;
        local=LocalState{};
        dungeon=DungeonState{};
        space=SpaceState{};
        pendingAttack=0;
        pendingAttackLine.clear();
        pendingAttackResolution=false; pendingAttackResolveMode=0;
        worldProjectileActive=false; worldProjectilePath.clear(); worldProjectileIndex=0;
        worldProjectileFramePresented=false;
        worldProjectileStepStartedMs=0;
        worldProjectileFirstSoundStartedMs=0;
        worldProjectileFirstSoundId=-1;
        worldPostHitSoundPending=false;
        worldPostHitSoundStarted=false;
        worldPostHitSoundStartedMs=0;
        enemyAttackQueue.clear();
        enemyAttackVisual=WorldEnemyAttackEvent{};
        enemyAttackVisualActive=false;
        enemyAttackHitFlash=false;
        enemyAttackVisualFramePresented=false;
        enemyAttackVisualIndex=0;
        enemyTurnTailPending=false;
        localActorDialoguePresentation.clear();
        localActorDialogueRows.clear();
        princessRescuePresentation.clear();
        princessRescuePopupLines.clear();
        landmarkPresentation.clear();
        landmarkPopupLines.clear();
        pendingWorldCastSpell=0; pendingWorldCastLine.clear(); commands.cancel();

        resurrection.beginAnimated(p);
        // OUT.EXE 0xC69F..0xC6D6 draws the skull first, then executes
        // sound(2) three times.  Only after those synchronous sounds return
        // does it print "<name>, thou art dead".
        resurrectionPhase=ResurrectionPhase::SkullIntro;
        resurrectionDeadShownMs=0;
        resurrectionStepMs=0;
        return true;
    };
    auto finishCommandTail=[&](){
        // OUT.EXE 0x8080: food is consumed only in outdoor mode. DS:3AA8 is
        // the saved transport index and DS:36F8 is the exact cadence table.
        if(!local.active && !dungeon.active && !space.active && !mondain.active &&
           !shuttleLaunchCountdownActive){
            static constexpr int FOOD_CADENCE[8]={2,3,3,4,5,6,11,0};
            const int transport=std::clamp<int>(p.transport,0,7);
            if(foodCycleCounter>=FOOD_CADENCE[static_cast<std::size_t>(transport)]){
                foodCycleCounter=0;
                if(p.food>0) --p.food;
            }
            ++foodCycleCounter;
        }

        resurrectIfNeeded();

        // OUT.EXE 0x90B9 resets its input-wait counter to zero after every
        // completed command before polling for the next key.
        overworldAnimEpochMs=SDL_GetTicks();
        overworldAnimProcessedStep=0;
        worldObjectAnimBit.fill(0);
        if(mondain.active && !mondain.introActive && !mondain.defeated && !mondain.victorious){
            mondainAnimEpochMs=SDL_GetTicks();
            mondainAnimProcessedStep=0;
        }
    };

    auto startNextEnemyAttackVisual=[&](){
        if(enemyAttackQueue.empty()) return;
        enemyAttackVisual=std::move(enemyAttackQueue.front());
        enemyAttackQueue.pop_front();
        enemyAttackVisualActive=true;
        enemyAttackHitFlash=false;
        enemyAttackVisualFramePresented=false;
        enemyAttackVisualIndex=0;
        enemyAttackVisualStartedMs=0;
        // OUT.EXE 0x84A1..0x84B7 draws tile 50 first and only then enters
        // synchronous sound(3).  SDL starts that sound after the first effect
        // frame has actually been presented, not before it exists on screen.

        // OUT.EXE 0x8465..0x8477 prints "<monster> attacks!" before the
        // projectile begins.  Keep that timing while retaining one Korean
        // logical row; the hit/miss suffix is appended after the animation.
        log.push(enemyAttackVisual.attackPrefix);
    };

    auto finishEnemyAttackVisual=[&](){
        // OUT.EXE applies the hit after the projectile reaches the player.
        // Keep the HUD/state change behind the visual, instead of exposing the
        // reduced HP before tile 50 has finished travelling.
        if(enemyAttackVisual.hit && enemyAttackVisual.damage>0){
            const int damage=enemyAttackVisual.damage;
            p.hits=static_cast<std::uint16_t>(
                damage>=static_cast<int>(p.hits)
                    ?0
                    :static_cast<int>(p.hits)-damage);
        }

        // The result belongs to the same logical command-window row as the
        // attack prefix, so no forced newline is introduced here.
        log.appendLast(enemyAttackVisual.resultSuffix);

        enemyAttackVisual=WorldEnemyAttackEvent{};
        enemyAttackVisualActive=false;
        enemyAttackHitFlash=false;
        enemyAttackVisualFramePresented=false;
        enemyAttackVisualIndex=0;

        if(!enemyAttackQueue.empty()){
            startNextEnemyAttackVisual();
        }else if(enemyTurnTailPending){
            enemyTurnTailPending=false;
            finishCommandTail();
        }
    };

    auto finishCommand=[&](){
        // OUT.EXE command loop is mode-dependent. M9 now runs the exact
        // dungeon monster movement/attack pass after each completed dungeon command.
        const bool outdoor=
            !local.active && !dungeon.active && !space.active && !mondain.active &&
            !shuttleLaunchCountdownActive;

        if(outdoor) overworld.maybeSpawn(p);
        ++p.steps;

        if(shuttleLaunchCountdownActive){
            // OUT.EXE chains directly into SPACE.EXE from Board; the outdoor
            // monster-response pass does not run while SPACE owns the launch
            // countdown.
        }else if(mondain.active){
            // All four Time Machine narration pages precede the final encounter
            // command loop; do not advance Mondain AI while any page owns
            // the screen and waits for Space/Return.
            if(!mondain.introActive){
                std::vector<std::string> mm;
                mondainSystem.updateAfterCommand(mondain,p,simulation,mm);
                pushMessages(mm);
            }
        }else if(space.active){
            std::vector<std::string> sm;
            spaceSystem.updateAfterCommand(space,p,sm);
            pushMessages(sm);
        }else if(local.active){
            if(updateLocal()) return;
        }else if(dungeon.active){
            std::vector<std::string> dm;
            dungeonSystem.updateAfterCommand(dungeon,p,dm);
            for(const auto& line:dm){
                if(line.find("의 공격!")!=std::string::npos){
                    // OUT.EXE 0x8A06: every adjacent dungeon monster attack
                    // begins with sound(3), even when it later misses or resolves
                    // as a monster-specific special effect.
                    dungeonEnemySfxQueue.push_back(sfxId(SfxEvent::EnemyRangedLaunch));
                    // OUT.EXE 0x8BBE: only the normal damaging hit path adds
                    // sound(2). Misses and armour/food/mental special effects do not.
                    if(line.find("적중!")!=std::string::npos)
                        dungeonEnemySfxQueue.push_back(sfxId(SfxEvent::CombatHit));
                }
            }
            pushMessages(dm);
            // OUT.EXE emits visible dungeon entity names as part of the redraw,
            // without a command prompt.  Our host redraws continuously, so emit
            // them exactly once at the command-boundary redraw instead of once
            // per SDL frame.
            if(dungeon.active) pushMessages(dungeonSystem.visibleObjectMessages(dungeon));
        }else{
            const std::uint16_t hitsBeforeEnemyPass=p.hits;
            const auto messages=overworld.updateAfterCommand(p);
            const auto events=overworld.takeEnemyAttackEvents();

            if(!events.empty()){
                p.hits=hitsBeforeEnemyPass;
                // Every message from this outdoor update corresponds 1:1 to an
                // enemy attack event.  Defer visible result text until its
                // projectile has run, matching OUT.EXE 0x83FC presentation.
                enemyAttackQueue.clear();
                for(const auto& ev:events) enemyAttackQueue.push_back(ev);
                enemyTurnTailPending=true;
                startNextEnemyAttackVisual();
                return;
            }

            pushMessages(messages);
        }

        finishCommandTail();
    };
    auto beginCommandSfxHold=[&](SfxEvent event,int continuation){
        commandSfxHoldPending=true;
        commandSfxHoldId=sfxId(event);
        commandSfxHoldContinuation=continuation;
    };
    auto finishCommandWithBeep=[&](){
        if(noiseEnabled) beginCommandSfxHold(SfxEvent::LocalBlocked,1);
        else finishCommand();
    };

    auto wrapX=[](int x){ x%=OriginalData::MAP_W; if(x<0)x+=OriginalData::MAP_W; return x; };
    auto wrapY=[](int y){ y%=OriginalData::MAP_H; if(y<0)y+=OriginalData::MAP_H; return y; };

    auto worldProjectileQuery=[&](int sx,int sy){
        const int wx=wrapX(static_cast<int>(p.x)+sx);
        const int wy=wrapY(static_cast<int>(p.y)+sy);
        for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i){
            const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
            if(o.empty()) continue;
            if(static_cast<int>(o.x())==wx && static_cast<int>(o.y())==wy)
                return static_cast<int>(o.tile());
        }
        return original.terrainAt(wx,wy);
    };

    auto resolveDelayedAttack=[&](){
        if(!pendingAttackLine.empty()){
            log.pushCommand(pendingAttackLine);
            pendingAttackLine.clear();
        }
        bool normalPlayerHit=false;
        if(pendingAttackResolveMode==1){
            if(local.active){
                pushMessages(simulation.attack(local,p,pendingAttackDx,pendingAttackDy));
                normalPlayerHit=simulation.lastPlayerAttackHit();
            }else{
                pushMessages(overworld.attack(p,pendingAttackDx,pendingAttackDy,WorldAttackMode::Normal));
                normalPlayerHit=overworld.lastPlayerAttackHit();
            }
        }else if(pendingAttackResolveMode==2){
            pushMessages(overworld.attack(p,pendingAttackDx,pendingAttackDy,WorldAttackMode::Fire));
        }else if(pendingAttackResolveMode==3){
            pushMessages(overworld.attack(p,pendingAttackDx,pendingAttackDy,WorldAttackMode::MagicMissile));
            normalPlayerHit=overworld.lastPlayerAttackHit();
        }else if(pendingAttackResolveMode==4){
            pushMessages(overworld.attack(p,pendingAttackDx,pendingAttackDy,WorldAttackMode::KillSpell));
            normalPlayerHit=overworld.lastPlayerAttackHit();
        }
        pendingAttackResolution=false;
        pendingAttackResolveMode=0;
        const bool wasWorldAttack=!local.active && !dungeon.active && !space.active && !mondain.active;
        worldProjectileActive=false;
        worldProjectilePath.clear();
        worldProjectileIndex=0;
        worldProjectileFramePresented=false;
        worldProjectileStepStartedMs=0;
        worldProjectileFirstSoundStartedMs=0;
        worldProjectileFirstSoundId=-1;

        // OUT.EXE returns from EF3 only after the terminal tile-50 cell has
        // been restored.  A successful field hit then calls sound(2)
        // synchronously, with no intervening delay.  On SDL we must first
        // present the frame in which tile 50 is gone, then start sound(2)
        // immediately after that present.  The enemy response is held until
        // this synchronous source sound has completed.
        if(normalPlayerHit && wasWorldAttack && noiseEnabled){
            worldPostHitSoundPending=true;
            worldPostHitSoundStarted=false;
            worldPostHitSoundStartedMs=0;
            return;
        }
        if(normalPlayerHit && noiseEnabled) sfx.play(SfxEvent::CombatHit);
        finishCommand();
    };

    std::cerr << "[runtime] video/assets/audio initialization complete; entering game loop\n";
    while(run){
#ifdef __DJGPP__
        // Audio runs only in the explicit SB16 test mode, never during SDL_Delay or BIOS polling.
        hf93AudioCaptureTick();
        static int hf93TraceLoop=0;
        if(!hf93TraceLoop++){FILE* f=fopen("LOOPTRACE.LOG","w");if(f){fputs("loop-enter\n",f);fclose(f);}}
#endif
        if(frontend.active()) frontend.update(SDL_GetTicks());

        // OUT.EXE 0x90B9: 1500 no-key polls, each costing delay(3), then
        // synthesize ASCII Space and pass it through the normal dispatcher.
        const std::uint32_t idleNow=SDL_GetTicks();
        const bool outMainCommandWait=
            !frontend.active() && !space.active && !mondain.active &&
            resurrectionPhase==ResurrectionPhase::Inactive &&
            !shuttleLaunchCountdownActive && !pendingQuitSave &&
            !showZ && !shop.active() && !king.active() && !commands.active() &&
            pendingAttack==0 && !pendingAttackResolution &&
            pendingWorldCastSpell==0 &&
            !worldProjectileActive && !worldPostHitSoundPending &&
            !enemyAttackVisualActive && !commandSfxHoldPending &&
            !localPlayerMoveSoundPending &&
            !localActorDialoguePresentation.active() &&
            !princessRescuePresentation.active() && !landmarkPresentation.active() &&
            !timeMachineUnreadyPopup;
        if(!outMainCommandWait){
            outIdlePassEpochMs=idleNow;
        }else if(static_cast<std::uint32_t>(idleNow-outIdlePassEpochMs)>=OUT_IDLE_AUTOPASS_MS){
            const int pendingKeys=
                SDL_PeepEvents(nullptr,0,SDL_PEEKEVENT,SDL_KEYDOWN,SDL_KEYDOWN);
            if(pendingKeys<=0){
                SDL_Event autoPass{};
                autoPass.type=SDL_KEYDOWN;
                autoPass.key.type=SDL_KEYDOWN;
                autoPass.key.keysym.sym=SDLK_SPACE;
                SDL_PushEvent(&autoPass);
            }
            outIdlePassEpochMs=idleNow;
        }

        // Test-only: after T has opened the real shop and its Buy/Sell prompt
        // has survived one outer-frame boundary, inject B for the next normal
        // event-dispatch pass.  This exercises the same public event path as a
        // player pressing B rather than bypassing ShopSystem.
        if(testTownArmourBuyBPending && shop.active() && shop.stage()==TradeStage::BuySell){
            SDL_Event be{}; be.type=SDL_KEYDOWN; be.key.type=SDL_KEYDOWN;
            be.key.keysym.sym=SDLK_b; SDL_PushEvent(&be);
            testTownArmourBuyBPending=false;
        }

#ifdef __DJGPP__
        static bool tracePreEvents=false;
        if(!tracePreEvents){tracePreEvents=true;FILE* f=fopen("LOOPTRAC.LOG","a");if(f){fputs("pre-events\n",f);fclose(f);}}
#endif
        if(haveSave){questJournal.attach(save,p);questJournal.update(p);}
        // Do not open help until the first gameplay frame has been visible
        // for 2000 ms. Block the countdown while unrelated modal screens own it.
        if(firstHelpPending && firstHelpWorldFrameMs!=0 &&
           !frontend.active() && !commandHelpPopup && !questPopup &&
           SDL_GetTicks()-firstHelpWorldFrameMs>=2000u){
            firstHelpPending=false;
            commandHelpPopup=true;
            commandHelpPresented=false;
            log.pushCommand("커맨드 도움말");
        }
        SDL_Event e;
        // Remember the last physical key and in-progress Hangul jamo across frames.
        static std::uint32_t lastPhysicalAlphaMs=0;
        static SDL_Keycode lastComposingKey=SDLK_UNKNOWN;

        // M231: sound() itself is synchronous and does not return to this loop
        // until the emulated source routine has completed.  No separate
        // SDL_GetTicks audio gate is needed.  Still dispatch at most one KEYDOWN
        // per outer frame so multiple BIOS/SDL key events cannot collapse into
        // one visual present.
        bool keyDispatchedThisFrame=false;
        while(!keyDispatchedThisFrame && SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT){run=false;break;}
            // KEYDOWN is preferred; Korean IMEs can consume it and only send
            // composition or committed text. Translate compatibility jamo.
            SDL_Keycode k=SDLK_UNKNOWN;
            SDL_Keymod mods=SDL_GetModState();
            if(e.type==SDL_KEYDOWN){
                k=e.key.keysym.sym;
                const SDL_Scancode scan=e.key.keysym.scancode;
                if(scan>=SDL_SCANCODE_A && scan<=SDL_SCANCODE_Z)
                    k=static_cast<SDL_Keycode>(SDLK_a+(scan-SDL_SCANCODE_A));
                mods=static_cast<SDL_Keymod>(e.key.keysym.mod);
                if(k>=SDLK_a && k<=SDLK_z)
                    lastPhysicalAlphaMs=SDL_GetTicks();
            }else if(e.type==SDL_TEXTINPUT || e.type==SDL_TEXTEDITING){
                const char* u=(e.type==SDL_TEXTINPUT)?e.text.text:e.edit.text;
                k=koreanJamoCommand(u);
                if(k==SDLK_UNKNOWN)continue;
                // Don't fire again when a physical KEYDOWN already handled it.
                if(SDL_GetTicks()-lastPhysicalAlphaMs<80)continue;
                if(e.type==SDL_TEXTEDITING){
                    // An IME can issue many composition updates for one key.
                    // De-duplicate identical jamo while the composition stays open.
                    if(k==lastComposingKey)continue;
                    lastComposingKey=k;
                }else{
                    if(k==lastComposingKey){lastComposingKey=SDLK_UNKNOWN;continue;}
                    lastComposingKey=SDLK_UNKNOWN;
                }
            }else continue;
            keyDispatchedThisFrame=true;
            outIdlePassEpochMs=SDL_GetTicks();
            const bool shifted=(mods & KMOD_SHIFT)!=0;
            // Apple II kill animation is synchronous: do not accept commands
            // while its front-view blink loop owns the screen.
            if(space.active && space.killBlinkUntilMs!=0 &&
               static_cast<std::int32_t>(SDL_GetTicks()-space.killBlinkUntilMs)<0)
                continue;

            if((mods & KMOD_ALT)!=0 && k==SDLK_RETURN){
                fullscreenActive=!fullscreenActive;
                SDL_SetWindowFullscreen(win,fullscreenActive?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
                if(!fullscreenActive)
                    SDL_SetWindowSize(win,videoConfig.windowWidth,videoConfig.windowHeight);
                continue;
            }

            if(shuttleLaunchCountdownActive){
                // SPACE.EXE 0x3C9C polls DOS input during every countdown step;
                // any returned key ends the remaining countdown and is consumed.
                shuttleLaunchSkipRequested=true;
                continue;
            }
            if(resurrectionPhase!=ResurrectionPhase::Inactive ||
               pendingQuitSave || pendingAttackResolution || worldProjectileActive || worldPostHitSoundPending ||
               commandSfxHoldPending || localPlayerMoveSoundPending || enemyAttackVisualActive ||
               localActorDialoguePresentation.active()) continue;
            if(timeMachineUnreadyPopup){
                // OUT.EXE 0x14A1 finishes drawing the shared 0x23C2 popup
                // before entering the any-key wait.  A key from the same host
                // frame must not erase a popup that has never been presented.
                if(!timeMachineUnreadyPresented) continue;
                timeMachineUnreadyPopup=false;
                timeMachineUnreadyPresented=false;
                finishCommand();
                continue;
            }
            if(princessRescuePresentation.active()){
                // Original rescue popup does not accept dismissal input until
                // after its initial delay(5). Any key then ends the popup.
                if(princessRescuePresentation.canDismiss(
                       SDL_GetTicks(),PRINCESS_POPUP_MIN_HOLD_MS)){
                    princessRescuePresentation.clear();
                    princessRescuePopupLines.clear();
                    local=LocalState{};
                    finishCommand();
                }
                continue;
            }
            if(landmarkPresentation.active()){
                // OUT.EXE cannot reach 0x2341 while its synchronous quest/reward
                // sound or success-frame restore is still running.  Preserve a
                // key typed during that work and consume it at the poll point.
                if(landmarkQuestSfxPending || landmarkSfxPending>=0 ||
                   landmarkBlueRestorePending){
                    landmarkDismissQueued=true;
                    continue;
                }
                if(landmarkPresentation.canDismiss(
                       SDL_GetTicks(),LANDMARK_POPUP_MIN_HOLD_MS)){
                    landmarkPresentation.clear();
                    landmarkPopupLines.clear();
                    landmarkDismissQueued=false;
                    finishCommand();
                }
                continue;
            }
            if(frontend.active()){
                const auto action=frontend.handleKey(k,static_cast<SDL_Keymod>(e.key.keysym.mod),p,haveSave);
                if(action==FrontendAction::Quit){ run=false; break; }
                if(action==FrontendAction::SaveCharacter){
                    const int slot=std::clamp(frontend.generationSlot(),0,3);
                    save=slotPaths[static_cast<std::size_t>(slot)];
                    if(!saveNewCharacter(save,p)){ std::cerr<<"Could not save generated character: "<<save<<"\n"; return 8; }
                    haveSave=true;
                    questJournal.resetForNewCharacter(save,p);
                    // A new character must receive help once on their first start.
                    // Automatic help is derived from saved XP; no marker files.
                    slotOccupied[static_cast<std::size_t>(slot)]=true;
                    slotNames[static_cast<std::size_t>(slot)]=p.name;
                    frontend.setRoster(slotOccupied,slotNames);
                } else if(action==FrontendAction::StartGame){
                    const int slot=std::clamp(frontend.selectedSlot(),0,3);
                    save=slotPaths[static_cast<std::size_t>(slot)];
                    std::string loadErr;
                    if(!original.loadSave(save,p,loadErr)){ std::cerr<<"Could not load selected character: "<<save<<"\n"; return 9; }
                    haveSave=true;
                    questJournal.path.clear(); // Reload journal for the selected slot.
                    simulation.seedFromDosTime(p.rngSeedWord);
                    // Show on each game launch with zero experience, without sidecar files.
                    firstHelpPending=(p.experience==0);
                    firstHelpWorldFrameMs=0;
                }
                continue;
            }

            if(questPopup){
                if(!questPopupPresented)continue;
                if(k==SDLK_SPACE){
                    questPopup=false;
                    questPopupPresented=false;
                    finishCommand(); // Same modal close/command prompt lifecycle as Ztats.
                }
                continue;
            }
            if(commandHelpPopup){
                // Host-only modal: the popup must be fully presented once before
                // its Space-only dismissal poll becomes active.
                if(!commandHelpPresented) continue;
                if(k==SDLK_SPACE){
                    commandHelpPopup=false;
                    commandHelpPresented=false;
                }
                continue;
            }

            const bool hostExtensionCommandReady=
                !showZ && !king.active() && !shop.active() && !commands.active() &&
                pendingAttack==0 && !pendingAttackResolution &&
                pendingWorldCastSpell==0 && pendingMondainCastSpell==0 &&
                !pendingMondainAttack && !pendingMondainAttackResolve &&
                !space.sectorScan && !(space.active && space.hyperPhase!=0) &&
                !(mondain.active && mondain.introActive) &&
                !(mondain.active && mondain.victorious);

            if(shifted && k==SDLK_q){
                // Reserved host-only Quest Status key.  Never fall through to
                // the original unshifted Q / Save Game command.
                if(hostExtensionCommandReady)
                    {
                        log.pushCommand("퀘스트 현황");
                        questJournal.attach(save,p);
                        questPopup=true;
                        questPopupPresented=false;
                    }
                continue;
            }
            if(hostExtensionCommandReady && shifted && k==SDLK_h){
                log.pushCommand("커맨드 도움말");
                commandHelpPopup=true;
                commandHelpPresented=false;
                continue;
            }
            if(hostExtensionCommandReady && shifted && k==SDLK_v){
                log.pushCommand("버전 정보");
                log.push("1.0K");
                continue;
            }

            if(king.active()){
                const KingStage before=king.stage();
                const std::string beforeLine=king.inputLine();

                if(before==KingStage::ServicePopup){
                    // The king popup is fully rendered before OUT.EXE reaches
                    // AD39's Space/Return poll.  Preserve that ordering across
                    // the host event loop.
                    if(!kingServicePopupPresented) continue;
                    char ch=0;
                    if(k==SDLK_SPACE) ch=' ';
                    else if(k==SDLK_RETURN||k==SDLK_KP_ENTER) ch='\r';
                    if(ch){
                        std::vector<std::string> m;
                        if(king.handleKey(ch,p,m)){
                            kingServicePopupPresented=false;
                            pushMessages(m);
                            finishCommand();
                        }
                    }
                    continue;
                }

                if(k==SDLK_ESCAPE){
                    std::vector<std::string> m; king.cancel(m);
                    log.push(beforeLine+"안 한다");
                    finishCommand();
                    continue;
                }
                char ch=0;
                if(k==SDLK_RETURN||k==SDLK_KP_ENTER) ch='\r';
                else if(k==SDLK_BACKSPACE) ch='\b';
                else if(k>=32 && k<=126) ch=static_cast<char>(k);
                if(ch){
                    std::vector<std::string> m;
                    const bool done=king.handleKey(ch,p,m);
                    if(before!=KingStage::ServicePopup &&
                       king.stage()==KingStage::ServicePopup)
                        kingServicePopupPresented=false;
                    if(before==KingStage::Choice){
                        const char u=static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                        if(u=='P') log.push(beforeLine+"공물");
                        else if(u=='S') log.push(beforeLine);
                        else if(done) log.push(beforeLine+"안 한다");
                        // P/neither are printed by OUT.EXE on the same modal
                        // line before it scrolls; don't duplicate the token as
                        // a separate host-log row.  Service quest text follows.
                        if(u=='S') pushMessages(m);
                    }else if(before==KingStage::PenceAmount){
                        if(done){ log.push(beforeLine); pushMessages(m); }
                    }
                    if(done){
                        // OUT.EXE 0x7C78: only the over-budget Pence path
                        // ("Thou hast not that much!") emits sound(1).
                        // Zero/cancel and successful donations are silent here.
                        const bool penceTooMuch=
                            before==KingStage::PenceAmount &&
                            std::find(m.begin(),m.end(),"그만한 돈이 없습니다!")!=m.end();
                        if(noiseEnabled && penceTooMuch)
                            beginCommandSfxHold(SfxEvent::LocalBlocked,1);
                        else
                            finishCommand();
                    }
                }
                continue;
            }
            if(shop.active()){
                const TradeStage beforeStage=shop.stage();
                const std::string beforeLine=shop.inputLine();
                const auto beforeOptions=shop.options();

                // Original shop result/message pages are fully drawn before the
                // blocking key wait begins.  Prevent a key already queued in
                // this SDL frame from dismissing an unseen popup.
                if(beforeStage==TradeStage::Message && !shopMessagePresented)
                    continue;

                if(k==SDLK_ESCAPE){
                    // A message popup is only being dismissed; the completed
                    // transaction row was already committed when it opened.
                    if(beforeStage==TradeStage::Message){
                        // OUT.EXE message waits accept a key and may continue to
                        // another modal page (notably pub post-drink seduction /
                        // rumor).  ESC therefore advances the modal sequence too;
                        // it must not bypass the original post-drink logic.
                        std::vector<std::string> m;
                        const bool done=shop.handleKey('\x1b',original,local,p,simulation,m);
                        if(shop.stage()==TradeStage::Message)
                            shopMessagePresented=false;
                        if(done){
                            shopMessagePresented=false;
                            if(!pendingShopResultLine.empty()){
                                log.pushCommand(pendingShopResultLine);
                                pendingShopResultLine.clear();
                            }
                            finishCommand();
                        }
                    }else{
                        std::vector<std::string> m; shop.cancel(m);
                        if(!beforeLine.empty()) log.pushCommand(beforeLine+"안 한다");
                        finishCommand();
                    }
                    continue;
                }
                char ch=0;
                if(k==SDLK_RETURN||k==SDLK_KP_ENTER) ch='\r';
                else if(k==SDLK_BACKSPACE) ch='\b';
                else if(k>=32 && k<=126) ch=static_cast<char>(k);
                if(ch){
                    std::string selectedName;
                    if(beforeStage==TradeStage::Item){
                        const char lc=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                        for(const auto& o:beforeOptions){
                            if(static_cast<char>(std::tolower(static_cast<unsigned char>(o.key)))==lc){
                                selectedName=o.name; break;
                            }
                        }
                    }

                    std::vector<std::string> m;
                    const bool done=shop.handleKey(ch,original,local,p,simulation,m);
                    const bool shopCantAfford=
                        std::find(m.begin(),m.end(),"돈이 부족합니다!")!=m.end();
                    if(shop.stage()==TradeStage::Message)
                        shopMessagePresented=false;

                    if(beforeStage==TradeStage::BuySell){
                        const char u=static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                        if(shop.stage()==TradeStage::Message){
                            // Keep the completed transaction on the live command
                            // row while the popup is visible.  It scrolls into
                            // history only when the popup is dismissed.
                            if(shop.kind()==ShopKind::Pub && u=='B')
                                pendingShopResultLine="거래하기-구입: 에일";
                            else
                                pendingShopResultLine=std::string("거래하기-")+(u=='B'?"구입: ":"판매: ")+"안 한다";
                        }else if(done){
                            if(u!='B'&&u!='S') log.pushCommand(beforeLine+"안 한다");
                            else log.pushCommand(std::string("거래하기-")+(u=='B'?"구입: ":"판매: ")+"안 한다");
                            for(const auto& x:m) if(x!="안 한다") log.push(x);
                            finishCommand();
                        }
                    }else if(beforeStage==TradeStage::Item && shop.stage()==TradeStage::Message){
                        // Original timing: Sold! opens while the completed buy text
                        // still occupies the current command row.  Only after the
                        // popup is dismissed does that row scroll up and a fresh
                        // command prompt appear.
                        pendingShopResultLine=beforeLine+(selectedName.empty()?"안 한다":selectedName);
                    }else if(beforeStage==TradeStage::Item && done){
                        if(selectedName.empty()){
                            log.pushCommand(beforeLine+"안 한다");
                            for(const auto& x:m) if(x!="안 한다") log.push(x);
                        }else{
                            log.pushCommand(beforeLine+selectedName);
                            std::size_t first=(!m.empty()&&m.front()==selectedName)?1u:0u;
                            for(std::size_t i=first;i<m.size();++i) log.push(m[i]);
                        }
                        // OUT.EXE 0x60A4/0x60BB: every shop purchase path that
                        // resolves as "Thou canst not afford it!" presents that
                        // result first, then runs sound(1).
                        if(noiseEnabled && shopCantAfford)
                            beginCommandSfxHold(SfxEvent::LocalBlocked,1);
                        else
                            finishCommand();
                    }else if(beforeStage==TradeStage::FoodQuantity && shop.stage()==TradeStage::Message){
                        if(!beforeLine.empty()){
                            const std::string prefix="거래하기-구입: ";
                            const std::string packs=beforeLine.rfind(prefix,0)==0 ? beforeLine.substr(prefix.size()) : std::string{};
                            pendingShopResultLine=prefix+"식량 "+packs+" 꾸러미";
                        }
                    }else if(beforeStage==TradeStage::FoodQuantity && done){
                        const bool foodCancelled=
                            std::find(m.begin(),m.end(),"안 산다")!=m.end();
                        if(!beforeLine.empty())
                            log.pushCommand(beforeLine+(foodCancelled?"안 산다":""));
                        if(!foodCancelled) pushMessages(m);
                        // OUT.EXE 0x60A4/0x60BB: insufficient funds -> sound(1).
                        if(noiseEnabled && shopCantAfford)
                            beginCommandSfxHold(SfxEvent::LocalBlocked,1);
                        else
                            finishCommand();
                    }else if(beforeStage==TradeStage::Message && done){
                        shopMessagePresented=false;
                        if(!pendingShopResultLine.empty()){
                            log.pushCommand(pendingShopResultLine);
                            pendingShopResultLine.clear();
                        }
                        finishCommand();
                    }
                }
                continue;
            }
            if(commands.active()){
                // OUT.EXE's Ready/Drop selector is drawn before its item-key
                // poll begins.  Do not let another SDL key queued in the same
                // host frame choose from a selector that has never been visible.
                if(commands.selectorPopupActive() && !selectorPopupPresented)
                    continue;
                if(k==SDLK_ESCAPE){
                    const CommandSystem::State before=commands.state();
                    std::string line=commands.inputLine();
                    commands.cancel();
                    selectorPopupPresented=false;
                    if(!line.empty()){
                        // DropPence can already contain typed digits; keep the
                        // cancellation word visually separated from that amount.
                        if(before==CommandSystem::State::DropPence &&
                           !line.empty() && line.back()!=' ')
                            line+=" ";
                        log.pushCommand(line+"안 한다");
                    }
                    finishCommand();
                    continue;
                }
                char ch=0;
                if(k==SDLK_RETURN||k==SDLK_KP_ENTER) ch='\r';
                else if(k==SDLK_BACKSPACE) ch='\b';
                else if(k>=32 && k<=126) ch=static_cast<char>(k);
                if(ch){
                    const CommandSystem::State before=commands.state();
                    const std::string beforeLine=commands.inputLine();
                    std::vector<std::string> cm;
                    const bool done=commands.handleKey(ch,local,p,simulation,cm);
                    if(commands.selectorPopupActive())
                        selectorPopupPresented=false;
                    if(done){
                        selectorPopupPresented=false;
                        const bool wasReady=
                            before==CommandSystem::State::ReadyCategory ||
                            before==CommandSystem::State::ReadyWeapon ||
                            before==CommandSystem::State::ReadyArmour ||
                            before==CommandSystem::State::ReadySpell;
                        const bool wasDropChoice=
                            before==CommandSystem::State::DropCategory ||
                            before==CommandSystem::State::DropWeapon ||
                            before==CommandSystem::State::DropArmour;

                        if((wasReady || wasDropChoice) && !beforeLine.empty()){
                            // OUT.EXE Ready category invalid-key path appends "nothing"
                            // to the same active command row.  For the localized Ready
                            // category, commit the concise command form requested by the
                            // project instead of preserving the W/A/S help text.
                            std::string finalLine=
                                (before==CommandSystem::State::ReadyCategory &&
                                 cm.size()==1 && cm.front()=="안 한다")
                                    ? std::string("장비하기:  ")
                                    : beforeLine;
                            if(!cm.empty()){
                                finalLine+=cm.front();
                                for(std::size_t i=1;i<cm.size();++i) log.push(cm[i]);
                            }
                            log.pushCommand(finalLine);
                        }else if(before==CommandSystem::State::DropPence && !beforeLine.empty()){
                            // Numeric Pence input is already embedded in beforeLine.
                            // "none" / zero is the one case that belongs on that
                            // same command row; real donation effects are outputs.
                            if(cm.size()==1 && cm.front()=="안 한다"){
                                log.pushCommand(beforeLine+cm.front());
                            }else{
                                log.pushCommand(beforeLine);
                                pushMessages(cm);
                            }
                        }else{
                            pushMessages(cm);
                        }
                        const bool positiveDropEffect=
                            before==CommandSystem::State::DropPence &&
                            (std::find(cm.begin(),cm.end(),"짜잔!")!=cm.end() ||
                             std::find(cm.begin(),cm.end(),"수리수리 마수리!")!=cm.end());
                        const bool failedDropPence=
                            before==CommandSystem::State::DropPence &&
                            std::find(cm.begin(),cm.end(),"그만한 돈이 없습니다!")!=cm.end();
                        if(noiseEnabled && failedDropPence)
                            beginCommandSfxHold(SfxEvent::LocalBlocked,1);
                        else if(noiseEnabled && positiveDropEffect)
                            beginCommandSfxHold(SfxEvent::PositiveEffect,1);
                        else
                            finishCommand();
                    }else{
                        pushMessages(cm);
                    }
                }
                continue;
            }
            if(pendingMondainAttack && mondain.active){
                int dir=0; std::string targetDir;
                if(k==SDLK_LEFT){dir=1;targetDir="서쪽";}
                else if(k==SDLK_RIGHT){dir=2;targetDir="동쪽";}
                else if(k==SDLK_UP){dir=3;targetDir="북쪽";}
                else if(k==SDLK_DOWN){dir=4;targetDir="남쪽";}

                // MONDAIN.EXE 0x09C0: a cardinal key is printed on the SAME
                // active Attack row, then the attack core begins with sound(7).
                // Defer resolution until after SDL_RenderPresent so both the
                // prompt marker and selected direction are actually visible.
                if(dir){
                    pendingMondainAttackLine+=targetDir;
                    pendingMondainAttackDir=dir;
                    pendingMondainAttack=false;
                    pendingMondainAttackResolve=true;
                }else{
                    // 0x09DC..0x09E9: any non-cardinal result prints "nothing"
                    // and executes sound(6).  Use the project's established
                    // localization for this direction-reader failure.
                    pendingMondainAttackLine+="안 한다";
                    pendingMondainAttack=false;
                    log.pushCommand(pendingMondainAttackLine);
                    pendingMondainAttackLine.clear();
                    if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                    else finishCommand();
                }
                continue;
            }
            if(pendingMondainCastSpell!=0 && mondain.active){
                int dir=0; std::string targetDir;
                if(k==SDLK_LEFT){dir=1;targetDir="서쪽";}
                else if(k==SDLK_RIGHT){dir=2;targetDir="동쪽";}
                else if(k==SDLK_UP){dir=3;targetDir="북쪽";}
                else if(k==SDLK_DOWN){dir=4;targetDir="남쪽";}
                const int sp=pendingMondainCastSpell;
                if(dir){
                    pendingMondainCastSpell=0;
                    log.pushCommand(pendingMondainCastLine+targetDir);
                    pendingMondainCastLine.clear();
                    if(sp>0 && p.spells[static_cast<std::size_t>(sp)]>0) --p.spells[static_cast<std::size_t>(sp)];
                    std::vector<std::string> mm;
                    mondainSystem.castSpell(mondain,p,simulation,sp,dir,mm);
                    pushMessages(mm); finishCommand();
                }else{
                    pendingMondainCastSpell=0;
                    log.pushCommand(pendingMondainCastLine+"안 한다");
                    pendingMondainCastLine.clear();
                    if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                    else finishCommand();
                }
                continue;
            }
            if(pendingWorldCastSpell!=0){
                // OUT.EXE 0x18FC prints the literal ": " and then calls the
                // direction reader while the Cast command is still the active
                // row.  The selected cardinal direction is printed on that same
                // row; the spell incantation/result starts on the next row.
                // Keep these names distinct from the main movement-dispatch dx/dy.
                int castDx=0,castDy=0; std::string targetDir;
                if(k==SDLK_LEFT){castDx=-1;targetDir="서쪽";}
                else if(k==SDLK_RIGHT){castDx=1;targetDir="동쪽";}
                else if(k==SDLK_UP){castDy=-1;targetDir="북쪽";}
                else if(k==SDLK_DOWN){castDy=1;targetDir="남쪽";}
                if(castDx||castDy){
                    const int sp=pendingWorldCastSpell;
                    pendingWorldCastSpell=0;
                    log.pushCommand(pendingWorldCastLine+targetDir);
                    pendingWorldCastLine.clear();
                    log.push(sp==3 ? "\"DELCIO-ERE-UI(역자주: 파괴)\"" : "\"INTERFICIO-NUNC!(역자주: 즉결 처형)\"");
                    // OUT.EXE spell-success gate: class 1 is automatic; every
                    // other class succeeds only when rng(1,100) < Wisdom+50.
                    if(p.klass!=1 && simulation.randomRange(1,100)>=static_cast<int>(p.wisdom)+50){
                        log.push("실패했습니다!");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                        else finishCommand();
                        continue;
                    }
                    pendingAttackResolveMode=(sp==3)?3:4;
                    pendingAttackDx=castDx; pendingAttackDy=castDy;
                    if(noiseEnabled) beginCommandSfxHold(SfxEvent::PositiveEffect,2);
                    else pendingAttackResolution=true;
                    continue;
                }
                // OUT.EXE direction reader reports Nothing on the same Cast row
                // and restores the just-consumed spell.
                const int sp=pendingWorldCastSpell;
                pendingWorldCastSpell=0;
                if(sp>0 && p.spells[static_cast<std::size_t>(sp)]<0xFFFFu)
                    ++p.spells[static_cast<std::size_t>(sp)];
                log.pushCommand(pendingWorldCastLine+"안 한다");
                pendingWorldCastLine.clear();
                // OUT.EXE 0x1B1E..0x1B4F: invalid/non-cardinal direction
                // restores the consumed spell and returns silently. No sound(6).
                finishCommand();
                continue;
            }
            if(showZ){
                // OUT.EXE C476 calls AD39 only after the current Inventory
                // page has been fully drawn.  Do not let an SDL key queued in
                // the same host frame skip a page before that page reaches
                // SDL_RenderPresent.
                if(!ztatsPagePresented) continue;
                // AD39 accepts only Space or Enter. Other keys, including
                // Escape, are ignored while Ztats owns input.
                if(k==SDLK_SPACE || k==SDLK_RETURN || k==SDLK_KP_ENTER){
                    if(ztatsHasMore(p,ztatsPage)){
                        ++ztatsPage;
                        ztatsPagePresented=false;
                    }else{
                        showZ=false;
                        ztatsPage=0;
                        ztatsPagePresented=false;
                        finishCommand();
                    }
                }
                continue;
            }
            if(pendingAttack!=0 &&
               k!=SDLK_LEFT && k!=SDLK_RIGHT && k!=SDLK_UP && k!=SDLK_DOWN){
                // OUT.EXE 1272 treats *every* non-direction key as "nothing".
                // This must happen before the host ESC shortcut so even Escape
                // consumes the pending Attack turn instead of leaving the game.
                pendingAttackLine+="안 한다";
                log.pushCommand(pendingAttackLine);
                pendingAttackLine.clear();
                pendingAttack=0;
                finishCommand();
                continue;
            }
            // SPACE.EXE 0x17B6..0x1826 owns keyboard input while Sector Scan is
            // visible.  Any accepted key dismisses the popup and is consumed;
            // even Escape must not leak through to the host quit shortcut.
            if(space.active && space.sectorScan){
                spaceSystem.dismissScan(space);
                continue;
            }
            // Gameplay ESC is not a host quit shortcut. Modal owners above
            // consume ESC when appropriate; otherwise the key is ignored here.
            // Window-close and the front-end Menu ESC/Q remain the explicit exits.
            if(k==SDLK_ESCAPE){ continue; }

            if(space.active){
                std::vector<std::string> sm;
                bool spaceBeep=false;
                // SPACE.EXE HyperJump is a blocking animation; keyboard polling resumes
                // only after all acceleration/starfield/deceleration phases complete.
                if(space.hyperPhase!=0) continue;
                // SPACE.EXE docking service reads the shared direction-key helper:
                // Left/Right/Up/Down are normalized to 1/2/3/4 and select the
                // corresponding left/right/top/bottom docking port.  While this
                // blocking menu is active, arrow keys must select a ship rather
                // than steer the currently docked craft.
                if(space.choosingShip){
                    int portCode=0;
                    if(k==SDLK_LEFT) portCode=1;
                    else if(k==SDLK_RIGHT) portCode=2;
                    else if(k==SDLK_UP) portCode=3;
                    else if(k==SDLK_DOWN) portCode=4;
                    if(portCode) spaceSystem.chooseShip(space,p,portCode,sm);
                    else sm.push_back("우주선을 선택하세요:");
                } else if(space.frontView && k>=SDLK_1 && k<=SDLK_8) spaceSystem.setSpeed(space,p,static_cast<int>(k-SDLK_0),sm);
                else if(k==SDLK_LEFT) spaceSystem.direction(space,p,1,sm);
                else if(k==SDLK_RIGHT) spaceSystem.direction(space,p,2,sm);
                else if(k==SDLK_UP || k==SDLK_DOWN){
                    // Approved parity rule: Top View Thrust and Retro replay
                    // exactly the same SFX event as ordinary overworld foot movement.
                    const bool topView=!space.frontView;
                    const int code=(k==SDLK_UP)?3:4;
                    
                    spaceSystem.direction(space,p,code,sm);
                    if(noiseEnabled && topView && sm.size()==1)
                        sfx.playImmediate(SfxEvent::OutdoorLandMove);
                }
                else if(k==SDLK_v) spaceSystem.toggleView(space,p,sm);
                else if(k==SDLK_i){
                    log.pushCommand("탐지하기");
                    spaceSystem.scan(space,sm);
                }
                else if(k==SDLK_h) spaceSystem.hyperJump(space,p,sm);
                else if(k==SDLK_f){
                    const bool frontBefore=space.frontView;
                    const std::uint32_t shotBefore=space.playerShotUntilMs;
                    spaceSystem.fire(space,p,sm);
                    if(!frontBefore) spaceBeep=true;
                    else if(space.playerShotUntilMs!=shotBefore) spaceFireSoundPending=true;
                }
                else if(k==SDLK_n){
                    noiseEnabled=!noiseEnabled;
                    sfx.setEnabled(noiseEnabled);
                    log.pushCommand(noiseEnabled ? "효과음 켜기" : "효과음 끄기");
                    finishCommand();
                    continue;
                }
                else if(k==SDLK_z){
                    log.pushCommand("상태창 보기");
                    ztatsPage=0;
                    showZ=true; ztatsPagePresented=false;
                    continue;
                }
                else if(k==SDLK_SPACE) spaceSystem.centerOrPass(space,sm);
                else if(k==SDLK_a){ spaceSystem.invalidCommand("공격하기?",sm); spaceBeep=true; }
                else if(k==SDLK_b){ spaceSystem.invalidCommand("타기?",sm); spaceBeep=true; }
                else if(k==SDLK_c){ spaceSystem.invalidCommand("시전하기?",sm); spaceBeep=true; }
                else if(k==SDLK_d){ spaceSystem.invalidCommand("버리기?",sm); spaceBeep=true; }
                else if(k==SDLK_e){ spaceSystem.invalidCommand("진입하기?",sm); spaceBeep=true; }
                else if(k==SDLK_g){ spaceSystem.invalidCommand("가지기?",sm); spaceBeep=true; }
                else if(k==SDLK_j){ spaceSystem.invalidCommand("뭐라고요?",sm); spaceBeep=true; }
                else if(k==SDLK_k){ spaceSystem.invalidCommand("오르기?",sm); spaceBeep=true; }
                else if(k==SDLK_l){ spaceSystem.invalidCommand("뭐라고요?",sm); spaceBeep=true; }
                else if(k==SDLK_m){ spaceSystem.invalidCommand("뭐라고요?",sm); spaceBeep=true; }
                else if(k==SDLK_o){ spaceSystem.invalidCommand("관 열기?",sm); spaceBeep=true; }
                else if(k==SDLK_p){ spaceSystem.invalidCommand("뭐라고요?",sm); spaceBeep=true; }
                else if(k==SDLK_q){ log.pushCommand("게임 종료하기?"); spaceBeep=true; }
                else if(k==SDLK_r){ spaceSystem.invalidCommand("장비하기?",sm); spaceBeep=true; }
                else if(k==SDLK_s){ spaceSystem.invalidCommand("훔치기?",sm); spaceBeep=true; }
                else if(k==SDLK_t){ spaceSystem.invalidCommand("거래하기?",sm); spaceBeep=true; }
                else if(k==SDLK_u){ spaceSystem.invalidCommand("자물쇠 열기?",sm); spaceBeep=true; }
                else if(k==SDLK_w){ spaceSystem.invalidCommand("뭐라고요?",sm); spaceBeep=true; }
                else if(k==SDLK_x){ spaceSystem.invalidCommand("내리기?",sm); spaceBeep=true; }
                else if(k==SDLK_y){ spaceSystem.invalidCommand("뭐라고요?",sm); spaceBeep=true; }
                else continue;
                pushMessages(sm);
                if(spaceBeep) finishCommandWithBeep();
                else finishCommand();
                continue;
            }

            if(mondain.active){
                if(mondain.introActive){
                    // MONDAIN.EXE image 0x1DE6 waits after each of four pages;
                    // the shared wait accepts only Return or Space.
                    if(mondain.introDrawPhase>=1 && (k==SDLK_SPACE || k==SDLK_RETURN)){
                        std::vector<std::string> mm;
                        mondainSystem.acknowledgeIntro(mondain,mm);
                        pushMessages(mm);
                    }
                    continue;
                }
                if(mondain.defeated){
                    // Original MONDAIN.EXE uses 50 BIOS ticks, not keyboard acknowledgement.
                    // Ignore ALL keyboard input until the timer expires.
                    continue;
                }
                if(mondain.victorious){
                    // M278: victory popup is deliberately non-interactive.
                    // Once the ending restart instruction appears, Space returns
                    // to the ULTIMA I title screen rather than invoking the host
                    // operating-system Ctrl-Alt-Delete sequence.
                    if(mondain.endingActive && mondain.endingRestartLineVisible && k==SDLK_SPACE){
                        mondain=MondainState{};
                        log.rows.clear();
                        showZ=false;
                        pendingAttack=0; pendingAttackLine.clear(); pendingAttackResolution=false;
                        pendingMondainAttack=false; pendingMondainAttackLine.clear(); pendingMondainAttackResolve=false;
                        pendingMondainCastSpell=0; pendingMondainCastLine.clear();
                        pendingWorldCastSpell=0; pendingWorldCastLine.clear();
                        commands=CommandSystem{};
                        noiseEnabled=true;
                        sfx.setEnabled(true); // M281: ending restart is a fresh game for SFX state too.
                        frontend.restartAtOpening();
                    }
                    continue;
                }
                std::vector<std::string> mm;
                bool mondainMoved=false;
                std::string mondainMoveDir;
                if(k==SDLK_LEFT){ mondainMoveDir="서"; mondainMoved=mondainSystem.move(mondain,-1,0,mm); }
                else if(k==SDLK_RIGHT){ mondainMoveDir="동"; mondainMoved=mondainSystem.move(mondain,1,0,mm); }
                else if(k==SDLK_UP){ mondainMoveDir="북"; mondainMoved=mondainSystem.move(mondain,0,-1,mm); }
                else if(k==SDLK_DOWN){ mondainMoveDir="남"; mondainMoved=mondainSystem.move(mondain,0,1,mm); }
                else if(k==SDLK_a){
                    const int weapon=std::clamp<int>(p.readyWeapon,0,15);
                    const std::string phrase=무기_공격_문구_한글(weapon);
                    // MONDAIN.EXE 0x0A3A..0x0A58 checks weapon range before
                    // entering the direction reader. Range zero prints '?' and
                    // sound(1); valid weapons keep the prompt live on BIOS row24.
                    if(OriginalData::weaponRange(weapon)<=0){
                        log.pushCommand(phrase+"?");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::InvalidAttackWeapon,1);
                        else finishCommand();
                        continue;
                    }
                    pendingMondainAttack=true;
                    pendingMondainAttackLine=phrase+":  ";
                    continue;
                }
                else if(k==SDLK_g){
                    mondainSystem.getGem(mondain,p,mm);
                    // MONDAIN keeps ordinary commands on the prompt-bearing row.
                    if(!mm.empty()){
                        log.pushCommand(mm.front());
                        for(std::size_t i=1;i<mm.size();++i) log.push(mm[i]);
                        mm.clear();
                    }
                }
                else if(k==SDLK_i){
                    mondainSystem.inform(mondain,mm);
                    if(!mm.empty()){
                        log.pushCommand(mm.front());
                        for(std::size_t i=1;i<mm.size();++i) log.push(mm[i]);
                        mm.clear();
                    }
                }
                else if(k==SDLK_SPACE){ log.pushCommand("통과"); finishCommand(); continue; }
                else if(k==SDLK_q){ log.pushCommand("게임 종료하기...는 불가능합니다!"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_c){
                    static constexpr const char* MONDAIN_SPELLS[11]={
                        "기도","관 열기","상자 열기","마탄","훔치기","상승 사다리",
                        "하강 사다리","순간이동","마력장 생성","마력장 파괴","죽음"};
                    const int sp=std::clamp<int>(p.readySpell,0,10);
                    const std::string row=std::string("시전하기-")+MONDAIN_SPELLS[sp];
                    if(sp!=0 && p.spells[static_cast<std::size_t>(sp)]==0){
                        log.pushCommand(row); log.push("그 주문은 재고가 없습니다!");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                        else finishCommand();
                        continue;
                    }
                    if(sp==8 || sp==9){
                        pendingMondainCastSpell=sp; pendingMondainCastLine=row+":  "; continue;
                    }
                    log.pushCommand(row);
                    if(sp!=0) --p.spells[static_cast<std::size_t>(sp)];
                    mondainSystem.castSpell(mondain,p,simulation,sp,0,mm);
                    pushMessages(mm); finishCommand(); continue;
                }
                else if(k==SDLK_r){
                    commands.beginReady(p,mm);
                    pushMessages(mm); continue;
                }
                else if(k==SDLK_s){
                    log.pushCommand("훔치기-보석");
                    log.push("불가능합니다!");
                    finishCommand(); continue;
                }
                else if(k==SDLK_t){
                    log.pushCommand("거래하기...몬데인은 협상에 응하지 않을 것입니다!");
                    finishCommand(); continue;
                }
                else if(k==SDLK_n){
                    noiseEnabled=!noiseEnabled;
                    sfx.setEnabled(noiseEnabled);
                    log.pushCommand(noiseEnabled ? "효과음 켜기" : "효과음 끄기");
                    finishCommand(); continue;
                }
                else if(k==SDLK_z){
                    log.pushCommand("상태창 보기");
                    ztatsPage=0; showZ=true; ztatsPagePresented=false;
                    continue;
                }
                else if(k>=SDLK_a && k<=SDLK_z){
                    // MONDAIN.EXE retains the alphabetic command dispatcher.
                    // Commands not yet given a final-arena implementation are still
                    // recognized, printed on a blue '>' command row, and consume the
                    // command just as an unavailable command does elsewhere in U1.
                    std::string text;
                    switch(k){
                        case SDLK_b:text="타기?";break;
                        case SDLK_d:text="버리기?";break;
                        case SDLK_e:text="진입하기?";break;
                        case SDLK_f:text="발포하기?";break;
                        case SDLK_h:text="하이퍼점프?";break;
                        case SDLK_j:text="뭐라고요?";break;
                        case SDLK_k:text="오르기?";break;
                        case SDLK_l:text="뭐라고요?";break;
                        case SDLK_m:text="뭐라고요?";break;
                        case SDLK_o:text="관 열기?";break;
                        case SDLK_p:text="뭐라고요?";break;
                        case SDLK_u:text="자물쇠 열기?";break;
                        case SDLK_v:text="시점 바꾸기?";break;
                        case SDLK_w:text="뭐라고요?";break;
                        case SDLK_x:text="내리기?";break;
                        case SDLK_y:text="뭐라고요?";break;
                        default:text="뭐라고요?";break;
                    }
                    log.pushCommand(text);
                    finishCommandWithBeep();
                    continue;
                }
                else continue;

                if(!mondainMoveDir.empty()){
                    if(mondainMoved){
                        // MONDAIN.EXE 0x0718..0x0796 prints the direction, blits
                        // the avatar into its new cell, then calls sound(4).
                        log.pushCommand(mondainMoveDir);
                        pendingMondainMoveFinish=true;
                    }else{
                        // MONDAIN.EXE 0x07B3: sound(0), negative palette,
                        // Burned + damage, sound(2), then restore palette.
                        if(noiseEnabled) sfx.play(0);
                        mondainCollisionFlashPending=true;
                        const int damage=std::max(1,static_cast<int>(p.hits)/10);
                        p.hits=static_cast<decltype(p.hits)>(std::max(0,static_cast<int>(p.hits)-damage));
                        log.pushCommand(mondainMoveDir);
                        log.push("화상을 입습니다! "+std::to_string(damage)+"의 피해량!");
                        if(p.hits==0){
                            mondain.defeated=true;
                            log.push("당신은 사망했습니다!");
                            mondainDefeatPopupStartedMs=0;
                        }else{
                            finishCommand();
                        }
                    }
                    continue;
                }
                pushMessages(mm); finishCommand(); continue;
            }

            if(dungeon.active){
                std::vector<std::string> dm;
                if(k==SDLK_LEFT) dungeonSystem.turnLeft(dungeon,dm);
                else if(k==SDLK_RIGHT) dungeonSystem.turnRight(dungeon,dm);
                else if(k==SDLK_UP) dungeonSystem.forward(dungeon,dm);
                else if(k==SDLK_DOWN) dungeonSystem.turnAround(dungeon,dm);
                else if(k==SDLK_i){
                    log.pushCommand("탐지하기");
                    dungeonSystem.searchSecret(dungeon,dm);
                }
                else if(k==SDLK_o){
                    dungeonSystem.openCoffin(dungeon,p,dm);
                    const bool noCoffin=!dm.empty() && dm.front().find("관이 없습니다!")!=std::string::npos;
                    if(!dm.empty()){
                        log.pushCommand(dm.front());
                        for(std::size_t i=1;i<dm.size();++i) log.push(dm[i]);
                    }
                    if(noCoffin) finishCommandWithBeep();
                    else finishCommand();
                    continue;
                }
                else if(k==SDLK_u){
                    dungeonSystem.unlockChest(dungeon,p,dm);
                    const bool noChest=!dm.empty() && dm.front().find("상자가 없습니다!")!=std::string::npos;
                    const bool trapTriggered=std::find(dm.begin(),dm.end(),"함정이 발동하였습니다!")!=dm.end();
                    if(!dm.empty()){
                        log.pushCommand(dm.front());
                        for(std::size_t i=1;i<dm.size();++i) log.push(dm[i]);
                    }
                    if(noChest) finishCommandWithBeep();
                    else if(noiseEnabled && trapTriggered) beginCommandSfxHold(SfxEvent::TrapTriggered,1);
                    else finishCommand();
                    continue;
                }
                else if(k==SDLK_v){ log.pushCommand("시점 바꾸기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_s){ log.pushCommand("훔치기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_g){ log.pushCommand("가지기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_h){ log.pushCommand("하이퍼점프?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_f){ log.pushCommand("발포하기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_c){
                    // OUT.EXE dungeon Cast: failures end in sound(6).  Successful
                    // spells use sound(5); Magic Missile/Kill route sound(5)
                    // through the attack helper and add sound(2) on an actual hit.
                    static constexpr const char* SPELLS[11]={
                        "기도","관 열기","상자 열기","마탄","훔치기","상승 사다리",
                        "하강 사다리","순간이동","생성","파괴","죽음"
                    };
                    static constexpr const char* INCANT[11]={
                        "POTENTIS-LAUDIS!(역자주: 강력한 찬양)",
                        "APERTUS!(역자주: 열려라)",
                        "PECUNIA!(역자주: 돈)",
                        "VASTO!(역자주: 파괴)",
                        "NUDO!",
                        "ASCENDO!(역자주: 상승)",
                        "INFERUS!(역자주: 지하)",
                        "DUCIS-EDUCO!(역자주: 밖으로 인도)",
                        "STRUXI!(역자주: 창조)",
                        "DELIO!(역자주: 제거)",
                        "INTERFICIO!(역자주: 처형)"
                    };
                    const int sp=std::clamp<int>(p.readySpell,0,10);
                    std::string castLine=std::string("시전하기-")+SPELLS[sp];
                    bool inventoryFailure=false;
                    if(sp==0){
                        castLine+=" \""+std::string(INCANT[sp])+"\"";
                        log.pushCommand(castLine);
                        dungeonSystem.castDungeon(dungeon,p,sp,dm);
                    }else if(p.spells[static_cast<std::size_t>(sp)]==0){
                        log.pushCommand(castLine);
                        dm.push_back("해당 주문이 소진되었습니다!");
                        inventoryFailure=true;
                    }else{
                        --p.spells[static_cast<std::size_t>(sp)];
                        castLine+=" \""+std::string(INCANT[sp])+"\"";
                        log.pushCommand(castLine);
                        dungeonSystem.castDungeon(dungeon,p,sp,dm);
                    }
                    pushMessages(dm);
                    if(noiseEnabled){
                        dungeonPlayerSfxQueue.clear();
                        const bool failed=inventoryFailure || dungeonSystem.lastCastFailed();
                        if(failed){
                            dungeonPlayerSfxQueue.push_back(sfxId(SfxEvent::NoEffect));
                        }else{
                            dungeonPlayerSfxQueue.push_back(sfxId(SfxEvent::PositiveEffect));
                            const int effective=dungeonSystem.lastCastEffectiveSpell();
                            if((effective==3 || effective==10) && dungeonSystem.lastCastHit())
                                dungeonPlayerSfxQueue.push_back(sfxId(SfxEvent::CombatHit));
                        }
                        dungeonPlayerSfxFinishPending=true;
                    }else finishCommand();
                    continue;
                }
                else if(k==SDLK_d){ log.pushCommand("버리기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_r){ commands.beginReady(p,dm); pushMessages(dm); continue; }
                else if(k==SDLK_k){
                    dungeonSystem.climb(dungeon,p,dm);
                    const bool noLadder=!dm.empty() && dm.front()=="무슨 사다리를 탑니까?";
                    if(!dm.empty()){
                        log.pushCommand(dm.front());
                        for(std::size_t i=1;i<dm.size();++i) log.push(dm[i]);
                    }
                    if(noLadder) finishCommandWithBeep();
                    else finishCommand();
                    continue;
                }
                else if(k==SDLK_a){
                    // OUT.EXE 0x12C2 checks the readied weapon range before the
                    // dungeon/local/world split.  Range 0 prints '?' after
                    // "Attack with <weapon>", calls sound(1), and returns without
                    // entering the dungeon orientation attack routine.
                    const int weapon=std::clamp<int>(p.readyWeapon,0,15);
                    const std::string phrase=무기_공격_문구_한글(weapon);
                    if(OriginalData::weaponRange(weapon)<=0){
                        log.pushCommand(phrase+"?");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::InvalidAttackWeapon,1);
                        else finishCommand();
                        continue;
                    }
                    // OUT.EXE dungeon attack routine 0xE52: sound(7) fires at
                    // 0xE61 before target/hit resolution. A successful hit then
                    // calls sound(2) at 0x1108 (survives) or 0x1187 (kill).
                    log.pushCommand(phrase);
                    const DungeonState dungeonBeforeAttack=dungeon;
                    dungeonSystem.attack(dungeon,p,dm);
                    bool dungeonLethal=false;
                    for(std::size_t ci=0;ci<dungeon.cells.size();++ci){
                        if(dungeonBeforeAttack.cells[ci].monster!=0xFFFFu &&
                           dungeon.cells[ci].monster==0xFFFFu){
                            dungeonLethal=true;
                            break;
                        }
                    }
                    if(dungeonLethal && dm.size()>1){
                        dungeonLethalVisualSnapshot=dungeonBeforeAttack;
                        dungeonLethalPrePresent=true;
                        dungeonLethalDeferred.assign(dm.begin()+1,dm.end());
                        dm.resize(1);
                    }
                    const bool dungeonPlayerHit=std::any_of(dm.begin(),dm.end(),[](const std::string& line){
                        return line.rfind("적중!",0)==0;
                    });
                    pushMessages(dm);
                    if(noiseEnabled){
                        dungeonPlayerSfxQueue.clear();
                        // Apple II DNG.bin: no target -> "Nothing" without attack sound.
                        // DOS edition plays a launch sound even on empty cells.
                        const bool dungeonHasTarget=std::none_of(dm.begin(),dm.end(),[](const std::string& line){
                            return line=="대상이 없습니다";
                        });
                        if(dungeonHasTarget) dungeonPlayerSfxQueue.push_back(sfxId(SfxEvent::PlayerAttackLaunch));
                        if(dungeonPlayerHit) dungeonPlayerSfxQueue.push_back(sfxId(SfxEvent::CombatHit));
                        dungeonPlayerSfxFinishPending=true;
                    }else if(!dungeonLethalPrePresent) finishCommand();
                    continue;
                }
                else if(k==SDLK_z){
                    log.pushCommand("상태창 보기");
                    ztatsPage=0;
                    showZ=true; ztatsPagePresented=false;
                    continue;
                }
                else if(k==SDLK_q){
                    log.pushCommand(quitIndoorsText());
                    finishCommandWithBeep();
                    continue;
                }
                else if(k==SDLK_SPACE) dm.push_back("통과");
                else if(k==SDLK_e){ log.pushCommand("진입하기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_t){ log.pushCommand("거래하기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_b){ log.pushCommand("타기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_x){ log.pushCommand("내리기?"); finishCommandWithBeep(); continue; }
                else if(k==SDLK_n){
                    noiseEnabled=!noiseEnabled;
                    sfx.setEnabled(noiseEnabled);
                    log.pushCommand(noiseEnabled ? "효과음 켜기" : "효과음 끄기");
                    finishCommand();
                    continue;
                }
                else if(k==SDLK_j || k==SDLK_l || k==SDLK_m || k==SDLK_p || k==SDLK_w || k==SDLK_y){
                    log.pushCommand("뭐라고요?");
                    finishCommandWithBeep();
                    continue;
                }
                else continue;
                // The four dungeon movement/turn verbs are commands, so their first
                // logical row carries the normal blue '>' marker. Forward's blocked
                // result is already appended to that same row by DungeonSystem::forward.
                if(k==SDLK_LEFT || k==SDLK_RIGHT || k==SDLK_UP || k==SDLK_DOWN){
                    if(!dm.empty()){
                        log.pushCommand(dm.front());
                        for(std::size_t i=1;i<dm.size();++i) log.push(dm[i]);
                    }
                }else{
                    pushMessages(dm);
                }
                finishCommand();
                continue;
            }

            int dx=0,dy=0; std::string moveDir,targetDir;
            if(k==SDLK_LEFT){dx=-1;moveDir="서";targetDir="서쪽";}
            else if(k==SDLK_RIGHT){dx=1;moveDir="동";targetDir="동쪽";}
            else if(k==SDLK_UP){dy=-1;moveDir="북";targetDir="북쪽";}
            else if(k==SDLK_DOWN){dy=1;moveDir="남";targetDir="남쪽";}

            if(dx||dy){
                if(pendingAttack!=0){
                    // OUT.EXE 1272 prints the chosen direction on the *same*
                    // active Attack/Fire command line.  Keep the user-requested
                    // two-space gap that is already part of the prompt.
                    pendingAttackLine+=targetDir;
                    pendingAttackResolveMode=pendingAttack;
                    pendingAttackDx=dx; pendingAttackDy=dy;
                    pendingAttack=0;
                    pendingAttackResolution=true;
                    continue;
                }

                log.pushCommand(moveDir);

        if(local.active){
                    const int nx=local.x+dx, ny=local.y+dy;
                    // OUT.EXE file 0x3076..0x30B3: leaving x=0..37/y=0..17 exits
                    // the local map. Raw TCD tile 0x3F is separately accepted as walkable.
                    if(nx<0||ny<0||nx>=OriginalData::LOCAL_W||ny>=OriginalData::LOCAL_H){
                        // OUT.EXE 0x309C..0x30AA checks for an adjacent Princess
                        // before discarding castle-local mode.  0x4DD6 then
                        // opens a dedicated popup over the castle screen.
                        if(!local.inTown){
                            auto rescue=simulation.rescuePrincess(local,p);
                            if(!rescue.empty()){
                                princessRescuePopupLines=std::move(rescue);
                                princessRescuePresentation.begin();
                                continue;
                            }
                        }
                        local=LocalState{};
                        finishCommand();
                        continue;
                    }
                    if(!simulation.passable(local,nx,ny) || simulation.actorAt(local,nx,ny)!=nullptr){
                        // M218 / OUT.EXE 0x3124..0x3131: a blocked town/castle
                        // move (wall or actor/NPC) uses the same sound(1) BEEP as
                        // an invalid X-it attempt on a landmark tile.
                        // The direction string is never printed; the failure text
                        // itself occupies the prompt-bearing command row.
                        log.replaceLastCommand("막혔습니다!");
                        if(noiseEnabled) sfx.playImmediate(SfxEvent::LocalBlocked);
                        finishCommand();
                        continue;
                    }
                    local.x=nx; local.y=ny;
                    // OUT.EXE 0x30E9..0x3117 restores the old cell, updates the
                    // coordinates, and blits tile 0x12 at the NEW cell BEFORE
                    // 0x3119 invokes sound(4).  Render that exact intermediate
                    // state first; the post-present path below then executes the
                    // synchronous PC-speaker routine and only afterwards runs
                    // the local actor turn via finishCommand().
                    if(noiseEnabled) localPlayerMoveSoundPending=true;
                    else finishCommand();
                    continue;
                }

                const int oldX=static_cast<int>(p.x), oldY=static_cast<int>(p.y);
                const int nx=wrapX(oldX+dx), ny=wrapY(oldY+dy);

                // User-requested facing rule for transports 1..5: horizontal INPUT sets
                // facing before collision/terrain checks, so a blocked attempt still turns
                // the vehicle.  North/south inputs preserve the previous horizontal facing.
                if(vehicleUsesHorizontalFacing(static_cast<int>(p.transport)))
                    vehicleFacingLeft=vehicleFacingLeftAfterInput(vehicleFacingLeft,dx,dy);

                if(const auto* mon=overworld.monsterAt(p,nx,ny)){
                    // OUT.EXE 0x073E..0x07CA builds "Blocked by a [monster]!"
                    // on the active command row and returns before the direction
                    // string at 0x08E0 can be printed.
                    log.replaceLastCommand(std::string(OverworldSimulation::monsterName(mon->tile()))+" 때문에 길이 막혔습니다!");
                    finishCommand();
                    continue;
                }
                std::vector<std::string> moveMessages;
                if(!vehicle.mayMove(original,p,nx,ny,moveMessages)){
                    // M218 / OUT.EXE 0x068A: every terrain/transport rejection
                    // routed through this helper prints its message and then calls
                    // sound(0).  Thus aircar-vs-woods, raft/frigate-vs-land and
                    // foot/horse/cart-vs-water all share one collision sound.
                    // (The same helper also covers mountains and other terrain
                    // transport mismatches; keep the mapping uniform.)
                    if(!moveMessages.empty()){
                        log.replaceLastCommand(moveMessages.front());
                        for(std::size_t i=1;i<moveMessages.size();++i) log.push(moveMessages[i]);
                    }
                    if(noiseEnabled) sfx.playImmediate(SfxEvent::OutdoorTerrainCollision);
                    finishCommand(); continue;
                }
                // OUT.EXE 0x08C4..0x08D2: ordinary outdoor movement uses
                // sound(4) only for transport indices 0..2 (foot/horse/cart).
                if(noiseEnabled && p.transport<=2) sfx.playImmediate(SfxEvent::OutdoorLandMove);
                p.x=static_cast<std::uint16_t>(nx);p.y=static_cast<std::uint16_t>(ny);
                p.worldObjects[0].w[2]=p.x; p.worldObjects[0].w[3]=p.y;
                overworld.onPlayerMoved(p,oldX,oldY);
                finishCommand();continue;
            }

            const bool inCastle=local.active && !local.inTown;

            switch(k){
                case SDLK_b: {
                    if(inCastle){ log.pushCommand("타기?"); finishCommandWithBeep(); break; }
                    if(local.active && local.inTown){ log.pushCommand("타기?"); finishCommandWithBeep(); break; }
                    const int before=static_cast<int>(p.transport);
                    std::vector<std::string> m; vehicle.board(p,local.active,m);
                    if(!m.empty()){
                        log.pushCommand(m.front());
                        for(std::size_t i=1;i<m.size();++i) log.push(m[i]);
                    }
                    const bool boardFailed = (p.transport==before) &&
                        (!m.empty() && (m.front()=="먼저 내려야 합니다!" || m.front()=="무엇을 탑니까?" || m.front()=="Board?"));
                    if(before!=static_cast<int>(p.transport) &&
                       vehicleUsesHorizontalFacing(static_cast<int>(p.transport)))
                        vehicleFacingLeft=vehicleInitialFacingLeft(static_cast<int>(p.transport));
                    if(before!=6 && p.transport==6){
                        dungeon=DungeonState{}; local=LocalState{};
                        // SPACE.EXE startup does not enter the flight UI
                        // immediately.  It first owns row24 for a 10-to-1
                        // countdown.  No ordinary command cursor is shown.
                        shuttleLaunchCountdownActive=true;
                        shuttleLaunchSkipRequested=false;
                        shuttleLaunchCountdownValue=10;
                        shuttleLaunchCountdownStepMs=SDL_GetTicks();
                        log.push("10....");
                    }
                    if(before!=7 && p.transport==7){
                        const bool fourGems=std::all_of(p.gems.begin(),p.gems.end(),[](std::uint16_t n){return n!=0;});
                        if(fourGems){ dungeon=DungeonState{}; local=LocalState{}; space=SpaceState{};
                            std::vector<std::string> mm; mondainSystem.enter(mondain,p,mm); pushMessages(mm); }
                        else {
                        timeMachineUnreadyPopup=true;
                        timeMachineUnreadyPresented=false;
                    }
                    }
                    if(boardFailed) finishCommandWithBeep();
                    else finishCommand();
                    break;
                }
                case SDLK_x: {
                    // M218: original X-it emits sound(1) for local-mode attempts,
                    // no-craft attempts, and invalid disembark terrain.  The user
                    // specifically confirmed the landmark X-it BEEP matches the
                    // wall/NPC collision BEEP, so keep both on one sound ID.
                    if(inCastle){
                        log.pushCommand("내리기?");
                        if(noiseEnabled) sfx.play(SfxEvent::InvalidXit);
                        finishCommand();
                        break;
                    }
                    if(local.active && local.inTown){
                        log.pushCommand("내리기?");
                        if(noiseEnabled) sfx.play(SfxEvent::InvalidXit);
                        finishCommand();
                        break;
                    }
                    std::vector<std::string> m;
                    vehicle.exitCraft(original,p,local.active,m);
                    const bool xitFailed = p.transport!=0;
                    if(!m.empty()){
                        log.pushCommand(m.front());
                        for(std::size_t i=1;i<m.size();++i) log.push(m[i]);
                    }
                    if(noiseEnabled && xitFailed) sfx.play(SfxEvent::InvalidXit);
                    finishCommand(); break;
                }
                case SDLK_a: {
                    // OUT.EXE 0x12C2: the normal Attack command uses DS:26DE as
                    // a capability/range table.  Range-0 inventory entries
                    // (rope, amulet, wand, staff) are Ready-able but not usable
                    // as normal Attack weapons: print '?' and call sound(1).
                    const int weapon=std::clamp<int>(p.readyWeapon,0,15);
                    const int wr=OriginalData::weaponRange(weapon);
                    const std::string phrase=무기_공격_문구_한글(weapon);
                    if(wr<=0){
                        log.pushCommand(phrase+"?");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::InvalidAttackWeapon,1);
                        else finishCommand();
                        break;
                    }
                    pendingAttack=1;
                    pendingAttackLine=phrase+":  ";
                    break;
                }
                case SDLK_c: {
                    // M146: local-map Cast is intentionally separate from the
                    // overworld and dungeon implementations.  In both castles
                    // and towns the original local command is a fixed no-effect
                    // line; it does not inspect Ready Spell and does not consume
                    // inventory.  Keep dungeon handling in its own earlier branch.
                    if(local.active){
                        log.pushCommand("시전하기 -- 흐음...아무런 효과도 없습니다!");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                        else finishCommand();
                        break;
                    }
                    // M141: OUT.EXE overworld Cast is driven by the spell selected
                    // through Ready Spell. Prayer is the index-0 sentinel and is
                    // never consumed; every real spell is consumed before its
                    // outdoor-vs-dungeon validation.
                    static constexpr const char* WORLD_SPELLS[11]={
                        "기도","관 열기","상자 열기","마탄","훔치기","상승 사다리",
                        "하강 사다리","순간이동","마력장 생성","마력장 파괴","죽음"};
                    const int sp=std::clamp<int>(p.readySpell,0,10);
                    const std::string castCommand=std::string("시전하기-")+WORLD_SPELLS[sp];
                    if(sp==0){
                        log.pushCommand(castCommand);
                        log.push("\"POTENTIS-LAUDIS!(역자주: 강력한 찬양)\"");
                        if(p.hits<15){
                            p.hits=15;
                            log.push("짜잔!");
                            if(noiseEnabled) beginCommandSfxHold(SfxEvent::PositiveEffect,1);
                        }else if(p.food<15){
                            p.food=15;
                            log.push("짜잔!");
                            if(noiseEnabled) beginCommandSfxHold(SfxEvent::PositiveEffect,1);
                        }else{
                            // Prayer's remaining outdoor branch attempts a divine
                            // monster removal. Keep the original shared DOS RNG;
                            // if the attempt finds no eligible nearby monster the
                            // visible result is the source no-effect string.
                            int victim=-1;
                            if(simulation.randomRange(1,100)<25){
                                for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i){
                                    const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
                                    if(o.empty() || !OverworldSimulation::isMonsterTile(static_cast<int>(o.tile()))) continue;
                                    int ddx=std::abs(static_cast<int>(o.x())-static_cast<int>(p.x));
                                    int ddy=std::abs(static_cast<int>(o.y())-static_cast<int>(p.y));
                                    const int prayerRange=OverworldSimulation::monsterRange(static_cast<int>(o.tile()));
                                    if(std::max(ddx,ddy)<=prayerRange){ victim=i; break; }
                                }
                            }
                            if(victim>=0){
                                for(int i=victim;i<OriginalData::WORLD_OBJECT_COUNT-1;++i)
                                    p.worldObjects[static_cast<std::size_t>(i)]=p.worldObjects[static_cast<std::size_t>(i+1)];
                                p.worldObjects.back().w.fill(0);
                                log.push("괴수가 사라졌습니다!");
                                if(noiseEnabled) beginCommandSfxHold(SfxEvent::PositiveEffect,1);
                            }else{
                                log.push("흐음...아무런 효과도 없습니다!");
                                if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                            }
                        }
                        if(!noiseEnabled) finishCommand();
                        break;
                    }
                    if(p.spells[static_cast<std::size_t>(sp)]==0){
                        log.pushCommand(castCommand);
                        log.push("그 주문은 재고가 없습니다!");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                        else finishCommand();
                        break;
                    }
                    --p.spells[static_cast<std::size_t>(sp)];
                    if(sp!=3 && sp!=10){
                        log.pushCommand(castCommand);
                        log.push("실패했습니다! 던전에서만 사용 가능합니다!");
                        if(noiseEnabled) beginCommandSfxHold(SfxEvent::NoEffect,1);
                        else finishCommand();
                        break;
                    }
                    // Source bytes at DS:0237 are ': ' (colon + one space).
                    // Keep this as a live row so the animated input cursor sits
                    // immediately to its right until a direction key is entered.
                    pendingWorldCastSpell=sp;
                    pendingWorldCastLine=castCommand+": ";
                    break;
                }
                case SDLK_d: {
                    if(!local.active){ log.pushCommand("버리기?"); finishCommandWithBeep(); break; }
                    std::vector<std::string> m; if(commands.beginDrop(local,p,m)){pushMessages(m);finishCommand();} else pushMessages(m); break;
                }
                case SDLK_r: {
                    std::vector<std::string> m; commands.beginReady(p,m); pushMessages(m); break;
                }
                case SDLK_f:
                    if(inCastle){ log.pushCommand("발포하기?"); finishCommandWithBeep(); break; }
                    if(local.active){ log.pushCommand("발포하기?"); finishCommandWithBeep(); break; }
                    if(p.transport!=4 && p.transport!=5){ log.pushCommand("무엇을 발포합니까?"); finishCommandWithBeep(); break; }
                    pendingAttackLine=(p.transport==4 ? "함포 발포하기:  " : "레이저포 발포하기:  ");
                    pendingAttack=2;
                    break;
                case SDLK_g:
                    if(inCastle){
                        const auto gm=simulation.get(local,p,storeroomAllowance);
                        if(!gm.empty()){
                            log.pushCommand(gm.front());
                            for(std::size_t i=1;i<gm.size();++i) log.push(gm[i]);
                        }
                        const bool castleGetBeep=!gm.empty() &&
                            (gm.front().find("여기는 가질 것이 없습니다!")!=std::string::npos ||
                             gm.front().find("국왕이 윤허하지 않았습니다!")!=std::string::npos);
                        if(castleGetBeep) finishCommandWithBeep();
                        else finishCommand();
                    }else if(local.active){
                        log.pushCommand("무엇을 가집니까?");
                        finishCommandWithBeep();
                    }else{
                        log.pushCommand("가지기?");
                        finishCommandWithBeep();
                    }
                    break;
                case SDLK_h:
                    // Outside SPACE, HyperJump is an invalid command and uses sound(1).
                    if(!local.active) log.pushCommand("하이퍼점프?");
                    else log.pushCommand(inCastle ? "하이퍼점프?" : "하이퍼점프?");
                    finishCommandWithBeep(); break;
                case SDLK_e:
                    if(local.active){ log.pushCommand(inCastle ? "진입하기?" : "어디에 진입합니까?"); finishCommandWithBeep(); break; }
                    if(const auto* loc=original.locationAt(p.x,p.y)){
                        if(loc->kind==LocationKind::Landmark){
                            log.pushCommand(locationEntryName(*loc)+"에 진입합니다...");
                            std::vector<std::string> m;
                            const int landmarkIndex=loc->id-41;
                            bool completesKingQuest=false;
                            if((landmarkIndex&1)!=0){
                                const int castleIndex=8-landmarkIndex;
                                completesKingQuest=
                                    p.kingQuest[static_cast<std::size_t>(castleIndex)]==0xFFFFu;
                            }
                            KingSystem::visitLandmark(
                                loc->id,p,lastLandmarkIndex,m,&landmarkSfxPending);
                            landmarkQuestSfxPending=noiseEnabled && completesKingQuest;
                            if(!noiseEnabled) landmarkSfxPending=-1;
                            landmarkBlueRestorePending=false;
                            landmarkDismissQueued=false;
                            landmarkPopupLines=std::move(m);
                            landmarkPresentation.begin();
                            continue;
                        }else if(loc->kind==LocationKind::Dungeon){
                            local=LocalState{};
                            std::vector<std::string> m;
                            dungeonSystem.enter(dungeon,p,loc->id,m);
                            // DungeonSystem retains its historical internal
                            // "Entering..." message for regression/oracle users;
                            // the localized UI replaces it with this one-line row.
                            if(!m.empty() && m.front()=="Entering...") m.erase(m.begin());
                            log.pushCommand(locationEntryName(*loc)+"에 진입합니다...");
                            pushMessages(m);
                        }else enterLocation(original,simulation,*loc,local,log);
                     } else log.pushCommand("진입하기?");
                    if(!local.active && original.locationAt(p.x,p.y)==nullptr) finishCommandWithBeep();
                    else finishCommand();
                    break;
                case SDLK_i:
                    if(inCastle){
                        // OUT.EXE 0x584B: "Inform and search" is printed first,
                        // then the castle name is emitted on the following row.
                        log.pushCommand("탐지하기");
                        const std::string castleName=성_탐지_결과(original,p);
                        if(!castleName.empty()) log.push(castleName+"입니다.");
                    }else if(!local.active){
                        log.pushCommand("탐지하기");
                        // M148: OUT.EXE outdoor Inform/Search first reports a named
                        // location under the player.  Only otherwise does it fall
                        // back to sea/woods/continent terrain information.
                        if(const LocationInfo* loc=original.locationAt(p.x,p.y)){
                            log.push(locationEntryName(*loc)+"입니다.");
                        }else{
                            const int terrain=original.terrainAt(p.x,p.y);
                            if(terrain==0){
                                log.push("바다입니다.");
                            }else if(terrain==2){
                                log.push("숲속입니다.");
                            }else{
                                const bool east=static_cast<int>(p.x)>=OriginalData::MAP_W/2;
                                const bool south=static_cast<int>(p.y)>=OriginalData::MAP_H/2;
                                if(!east && !south) log.push("로드 브리티시의 땅입니다.");
                                else if(east && !south) log.push("봉건 영주들의 땅입니다.");
                                else if(!east && south) log.push("어두운 미지의 땅입니다.");
                                else log.push("위험과 절망의 땅입니다.");
                            }
                        }
                    }else{
                        // M151 town-local Inform/Search: report the current city
                        // name just as the castle/local branch reports its name.
                        log.pushCommand("탐지하기");
                        for(const auto& loc:original.locations()){
                            if(loc.id==local.locationId){
                                log.push(locationEntryName(loc)+"입니다.");
                                break;
                            }
                        }
                    }
                    finishCommand();
                    break;
                case SDLK_t: {
                    if(!local.active){ log.pushCommand("거래하기?"); finishCommandWithBeep(); break; }
                    std::vector<std::string> m;
                    if(local.active && !local.inTown){
                        const bool opened=king.begin(original,local,p,storeroomAllowance,m);
                        if(opened && king.stage()==KingStage::ServicePopup)
                            kingServicePopupPresented=false;
                        if(!m.empty()){
                            // OUT.EXE 0x7B63 first prints "Transact with king"
                            // onto the active command line, so it retains the
                            // blue prompt when scrolled.  Subsequent king text
                            // is ordinary output without another prompt.
                            log.pushCommand(m.front());
                            // Completed-quest reward text belongs exclusively
                            // to the original 0x23C2 popup, not to the command
                            // log. Ordinary transaction output still follows.
                            if(!king.rewardPopup())
                                for(std::size_t i=1;i<m.size();++i) log.push(m[i]);
                        }
                        if(!opened){
                            const bool kingBeep=!m.empty() && m.size()>=2 &&
                                (m.back()=="국왕이 여기 없습니다!" || m.back()=="왕이 당신의 알현을 거절합니다!");
                            if(kingBeep){ finishCommandWithBeep(); }
                            else finishCommand();
                        }
                    }else{
                        const bool opened=shop.begin(original,local,p,m);
                        if(!opened){
                            if(!m.empty()){
                                log.pushCommand(m.front());
                                for(std::size_t i=1;i<m.size();++i) log.push(m[i]);
                            }
                            const bool shopBeep=!m.empty() &&
                                ((m.size()>=2 && m.back()=="판매대 옆으로 가야 합니다!") ||
                                 m.front()=="거래하기?" || m.front()=="뭐라고요?");
                            if(shopBeep) finishCommandWithBeep();
                            else finishCommand();
                        }
                        // When opened, OUT.EXE keeps "Transact-Buy, Sell: " as
                        // the live command row.  Do not push a separate log row.
                    }
                    break;
                }
                case SDLK_o:
                    if(!local.active) log.pushCommand("관 열기?");
                    else log.pushCommand(inCastle ? "관 열기?" : "관 열기?");
                    finishCommandWithBeep(); break;
                case SDLK_s: {
                    if(local.active){
                        const auto sm=simulation.steal(local,p);
                        const bool notStoreroom=!sm.empty() && sm.front().find("여기는 훔칠 것이 없습니다!")!=std::string::npos;
                        if(!sm.empty()){
                            log.pushCommand(sm.front());
                            for(std::size_t i=1;i<sm.size();++i) log.push(sm[i]);
                        }
                        if(notStoreroom) finishCommandWithBeep();
                        else finishCommand();
                    }else{
                        log.pushCommand("훔치기?");
                        finishCommandWithBeep();
                    }
                    break;
                }
                case SDLK_u: {
                    if(local.active){
                        const auto um=simulation.unlock(local,p);
                        const bool noPrisonDoor=(um.size()==1 && um.front()=="무슨 자물쇠를 엽니까?");
                        if(!um.empty()){
                            log.pushCommand(um.front());
                            for(std::size_t i=1;i<um.size();++i) log.push(um[i]);
                        }
                        if(noPrisonDoor) finishCommandWithBeep();
                        else finishCommand();
                    }else{
                        log.pushCommand("자물쇠 열기?");
                        finishCommandWithBeep();
                    }
                    break;
                }
                case SDLK_v:
                    if(!local.active) log.pushCommand("시점 바꾸기?");
                    else log.pushCommand(inCastle ? "시점 바꾸기?" : "시점 바꾸기?");
                    finishCommandWithBeep(); break;
                case SDLK_k:
                    if(!local.active) log.pushCommand("오르기?");
                    else log.pushCommand(inCastle ? "오르기?" : "오르기?");
                    finishCommandWithBeep(); break;
                case SDLK_n:
                    noiseEnabled=!noiseEnabled;
                    sfx.setEnabled(noiseEnabled);
                    if(inCastle) log.pushCommand(noiseEnabled ? "효과음 켜기" : "효과음 끄기");
                    else if(!local.active) log.pushCommand(noiseEnabled ? "효과음 켜기" : "효과음 끄기");
                    else log.pushCommand(noiseEnabled ? "효과음 켜기" : "효과음 끄기");
                    finishCommand(); break;
                case SDLK_j:
                case SDLK_l:
                case SDLK_m:
                case SDLK_p:
                case SDLK_w:
                case SDLK_y:
                    if(inCastle || !local.active || (local.active&&local.inTown)){ log.pushCommand("뭐라고요?"); finishCommandWithBeep(); }
                    break;
                case SDLK_q:
                    if(!quitSaveAllowedOutdoors(local.active,dungeon.active,space.active,mondain.active)){
                        log.pushCommand(quitIndoorsText());
                        finishCommandWithBeep();
                    }else{
                        log.pushCommand(quitSavingText());
                        pendingQuitSave=true;
                        quitSaveStartedMs=SDL_GetTicks();
                    }
                    break;
                case SDLK_z:
                    log.pushCommand("상태창 보기");
                    ztatsPage=0;
                    showZ=true; ztatsPagePresented=false;
                    break;
                case SDLK_SPACE: log.pushCommand("통과"); finishCommand(); break;
                default: break;
            }
        }

        if(pendingQuitSave &&
           static_cast<std::uint32_t>(SDL_GetTicks()-quitSaveStartedMs)>=QUIT_SAVE_VISIBLE_DELAY_MS){
            const bool saved=saveState(save,p);
            if(saved){
                firstHelpPending=false;
                firstHelpWorldFrameMs=0;
            }
            if(saved){questJournal.attach(save,p);questJournal.update(p);questJournal.persist();}
            log.appendLast(saved?quitSavedSuffix():"저장에 실패했습니다.");
            pendingQuitSave=false;
            finishCommand();
        }

        if(pendingAttackResolution && !worldProjectileActive){
            // M221: no invented 300-ms pause exists here. OUT.EXE accepts the
            // direction and enters the attack routine immediately.
            if(local.active && pendingAttackResolveMode==1){
                // Local/castle Attack calls sound(7) before its target scan.
                if(noiseEnabled) sfx.play(SfxEvent::PlayerAttackLaunch);
                resolveDelayedAttack();
            }else if(!local.active && !dungeon.active && !space.active && !mondain.active){
                // OUT.EXE EF3: draw physical tile 50 in the adjacent cell first.
                // On distance 1, normal Attack then calls sound(7) synchronously
                // while that same first cell remains drawn. Only after the call
                // returns does a ranged attack advance to later cells, each with
                // delay(2).
                const int range=(pendingAttackResolveMode==2 || pendingAttackResolveMode==3 || pendingAttackResolveMode==4)
                    ?3:OriginalData::weaponRange(static_cast<int>(p.readyWeapon));
                worldProjectilePath=buildWorldProjectileVisualPath(
                    pendingAttackDx,pendingAttackDy,range,worldProjectileQuery);
                if(!worldProjectilePath.empty()){
                    worldProjectileActive=true;
                    worldProjectileIndex=0;
                    worldProjectileFramePresented=false;
                    worldProjectileStepStartedMs=0;
                    worldProjectileFirstSoundStartedMs=0;
                    // OUT.EXE 0x1BF0 passes 7 into EF3 for both field
                    // Magic Missile and Kill, so their first projectile cell
                    // uses the same sound(7) launch cue as a normal player shot.
                    worldProjectileFirstSoundId=
                        (pendingAttackResolveMode==1 || pendingAttackResolveMode==3 || pendingAttackResolveMode==4)
                            ?sfxId(SfxEvent::PlayerAttackLaunch)
                            :(pendingAttackResolveMode==2 ? sfxId(SfxEvent::VehicleFire) : -1);
                }else{
                    resolveDelayedAttack();
                }
            }else{
                resolveDelayedAttack();
            }
        }

#ifdef __DJGPP__
        static bool traceMid=false;
        if(!traceMid){traceMid=true;FILE* f=fopen("LOOPTRAC.LOG","a");if(f){fputs("pre-projectile\n",f);fclose(f);}}
#endif
        if(worldProjectileActive && worldProjectileFramePresented){
            bool advance=false;
            if(worldProjectileIndex==0 && worldProjectileFirstSoundId>=0){
                // M231 calls the reconstructed sound() synchronously immediately
                // after the first-cell frame is presented.  Reaching this point
                // therefore already means the source routine has returned.
                advance=true;
            }else{
                // EF3 later cells: restore/advance only after delay(2), measured
                // from the frame in which this cell was actually presented.
                advance=static_cast<std::uint32_t>(
                    SDL_GetTicks()-worldProjectileStepStartedMs)>=WORLD_PROJECTILE_STEP_MS;
            }

            if(advance){
                ++worldProjectileIndex;
                worldProjectileFramePresented=false;
                if(worldProjectileIndex>=worldProjectilePath.size()){
                    resolveDelayedAttack();
                }
            }
        }

        if(enemyAttackVisualActive){
            bool advance=false;

            if(enemyAttackHitFlash){
                // The hit-flash frame was presented and sound(2) completed
                // synchronously in the post-present path below.
                if(enemyAttackVisualFramePresented) finishEnemyAttackVisual();
            }else if(enemyAttackVisualIndex==0){
                // First projectile cell likewise cannot become frame-presented
                // until the post-present sound(3) call has returned.
                advance=enemyAttackVisualFramePresented;
            }else{
                // OUT.EXE 0x84BE..0x84C5: later cells wait delay(2), beginning
                // only after the new cell has actually been presented.
                advance=enemyAttackVisualFramePresented &&
                    static_cast<std::uint32_t>(
                        SDL_GetTicks()-enemyAttackVisualStartedMs)>=WORLD_PROJECTILE_STEP_MS;
            }

            if(enemyAttackVisualActive && !enemyAttackHitFlash && advance){
                ++enemyAttackVisualIndex;
                enemyAttackVisualFramePresented=false;
                enemyAttackVisualStartedMs=0;

                if(enemyAttackVisualIndex>=enemyAttackVisual.path.size()){
                    if(enemyAttackVisual.hit){
                        enemyAttackHitFlash=true;
                        enemyAttackVisualFramePresented=false;
                    }else{
                        finishEnemyAttackVisual();
                    }
                }
            }
        }

#ifdef __DJGPP__
        static bool tracePreDraw=false;
        if(!tracePreDraw){tracePreDraw=true;FILE* f=fopen("LOOPTRAC.LOG","a");if(f){fputs("pre-draw\n",f);fclose(f);}}
#endif
        if(frontend.active()){
#ifdef __DJGPP__
            static int hf93TraceDraw=0;
            if(!hf93TraceDraw++){FILE* f=fopen("LOOPTRACE.LOG","a");if(f){fputs("frontend-draw-enter\n",f);fclose(f);}}
#endif
            frontend.draw(r,font,original,haveSave);
            if(!captureBmp.empty()){
                std::string capErr;
                if(!saveRendererBmp(r,captureBmp,W,H,capErr)){std::cerr<<capErr<<"\n";return 6;}
                captureBmp.clear();
                if(captureOnce) run=false;
            }
            const bool enterAfterPresent=frontend.enterGamePending();
            presentFrame();
            if(enterAfterPresent){
                // M77: keep the localized loading card visible for a short,
                // deliberate beat after it has actually reached the screen.
                // 300 ms is long enough to read without making startup feel slow.
                constexpr std::uint32_t LOADING_HOLD_MS=300;
                SDL_Delay(LOADING_HOLD_MS);
                frontend.finishLoading();
            }
            continue;
        }
        if(shuttleLaunchCountdownActive){
            const std::uint32_t now=SDL_GetTicks();
            auto finishShuttleLaunch=[&](){
                shuttleLaunchCountdownActive=false;
                shuttleLaunchSkipRequested=false;
                shuttleLaunchCountdownValue=0;
                shuttleLaunchCountdownStepMs=0;
                std::vector<std::string> sm;
                spaceSystem.enter(space,p,sm);
                pushMessages(sm);
                // SPACE.EXE's startup-fatal path can return immediately to
                // OUT.EXE with Hits/Food zero; begin resurrection without
                // waiting for another keypress.
                resurrectIfNeeded();
            };

            if(shuttleLaunchSkipRequested){
                finishShuttleLaunch();
            }else{
                while(shuttleLaunchCountdownActive &&
                      static_cast<std::uint32_t>(now-shuttleLaunchCountdownStepMs)>=SHUTTLE_COUNTDOWN_STEP_MS){
                    shuttleLaunchCountdownStepMs+=SHUTTLE_COUNTDOWN_STEP_MS;
                    --shuttleLaunchCountdownValue;
                    if(shuttleLaunchCountdownValue>0){
                        log.appendLast(std::to_string(shuttleLaunchCountdownValue)+"....");
                    }else{
                        finishShuttleLaunch();
                    }
                }
            }
        }

        if(space.active){
            std::vector<std::string> sm;
            const std::uint32_t now=SDL_GetTicks();
            int runSpaceTicks=0;
            if(showZ){
                // Ztats is a blocking/pause screen in the original command set.
                // Freeze SPACE flight while the inventory/status pages own input.
                spaceTopTimingActive=false;
                spaceFrontTimingActive=false;
                spaceFrontRetraceBudget=0;
                spaceHyperTimingActive=false;
                spaceHyperRetraceBudget=0;
            }else if(space.sectorScan){
                // Sector Scan is blocking in SPACE.EXE. Freeze flight while the
                // popup owns input, and restart timing cleanly after dismissal.
                spaceTopTimingActive=false;
                spaceFrontTimingActive=false;
                spaceFrontRetraceBudget=0;
                spaceHyperTimingActive=false;
                spaceHyperRetraceBudget=0;
            }else if(space.hyperPhase!=0 && noiseEnabled && sfx.available() && sfx.enabled()){
                // HF63: CPU audio snapshot controls the on-screen stage and
                // number of $9497 star frames; no second independent scheduler.
                spaceTopTimingActive=false;
                spaceFrontTimingActive=false;
                spaceHyperTimingActive=false;
                spaceSystem.updateAppleHyperProgress(space,p,sm,sfx.appleHyperProgress());
            }else if(space.hyperPhase!=0){
                // M252: source HyperJump does not advance phase 1/4 once per host
                // frame. Each source update pays the speed-indexed DS:0C9E
                // vertical-retrace wait (0,8,7,6,5,4,3,2,1 at speeds 0..8).
                // The first update is immediate, exactly like entering 0x3530;
                // subsequent updates wait for the retraces charged by the prior
                // source update. Phase 2/3 has no explicit retrace wait.
                spaceTopTimingActive=false;
                spaceFrontTimingActive=false;
                spaceFrontRetraceBudget=0;
                if(!spaceHyperTimingActive){
                    spaceHyperTimingActive=true;
                    spaceHyperClockMs=now;
                    spaceHyperRetraceBudget=0;
                    runSpaceTicks=1;
                }else{
                    const std::uint32_t delta=static_cast<std::uint32_t>(now-spaceHyperClockMs);
                    spaceHyperClockMs=now;
                    if(delta>250u) spaceHyperRetraceBudget=0;
                    else spaceHyperRetraceBudget+=static_cast<std::uint64_t>(delta)*SPACE_FRONT_RETRACE_HZ;
                    const std::uint64_t hyperThreshold=
                        static_cast<std::uint64_t>(std::max(0,space.hyperWaitRetraces))*1000u;
                    if(hyperThreshold==0u) runSpaceTicks=1;
                    else if(spaceHyperRetraceBudget>=hyperThreshold){
                        spaceHyperRetraceBudget-=hyperThreshold;
                        runSpaceTicks=1;
                    }
                }
            }else if(space.frontView){
                spaceHyperTimingActive=false;
                spaceHyperRetraceBudget=0;
                spaceTopTimingActive=false;
                int speed=space.frontSpeed;
                if(speed<1)speed=1; else if(speed>8)speed=8;
                const std::uint64_t threshold=
                    static_cast<std::uint64_t>(SPACE_FRONT_RETRACE_COUNTS[speed])*1000u;
                if(!spaceFrontTimingActive){
                    spaceFrontTimingActive=true;
                    spaceFrontClockMs=now;
                    spaceFrontRetraceBudget=0;
                }else{
                    const std::uint32_t delta=static_cast<std::uint32_t>(now-spaceFrontClockMs);
                    spaceFrontClockMs=now;
                    // Accumulate 70 retrace-units per millisecond instead of
                    // rounding every wait to whole host frames.  This preserves
                    // the original average 70/retraceCount flight rate even on
                    // a 60-Hz SDL presentation.  Drop very long host stalls.
                    if(delta>250u) spaceFrontRetraceBudget=0;
                    else spaceFrontRetraceBudget+=static_cast<std::uint64_t>(delta)*SPACE_FRONT_RETRACE_HZ;
                    while(spaceFrontRetraceBudget>=threshold && runSpaceTicks<4){
                        spaceFrontRetraceBudget-=threshold;
                        ++runSpaceTicks;
                    }
                    if(runSpaceTicks==4 && spaceFrontRetraceBudget>=threshold)
                        spaceFrontRetraceBudget=0;
                }
            }else if(!spaceTopTimingActive){
                spaceFrontTimingActive=false;
                spaceFrontRetraceBudget=0;
                spaceTopTimingActive=true;
                spaceTopMotionTickMs=now;
            }else if(static_cast<std::uint32_t>(now-spaceTopMotionTickMs)>=SPACE_TOP_MOTION_TICK_MS){
                // Drop excess elapsed wall time rather than replaying many
                // movement steps after a debugger stop/window stall.
                spaceTopMotionTickMs=now;
                runSpaceTicks=1;
            }
            for(int i=0;i<runSpaceTicks && space.active;++i)
                spaceSystem.updateFrame(space,p,sm);
            if(space.hyperPhase==0 && spaceHyperTimingActive){
                spaceHyperTimingActive=false;
                spaceHyperRetraceBudget=0;
                spaceFrontTimingActive=false;
                spaceFrontRetraceBudget=0;
            }
            if(runSpaceTicks>0 && space.active){
                // HF47: ordinary Front View uses the Apple II STAR.OBJ/FGT3
                // speaker state machine. HyperJump keeps DOS sound(9).
                const int pulses=runSpaceTicks*2;
                // HF50: the tight Apple II DE=1 loops in phases 2/3
                // are synthesized from STAR/FGT3, not queued DOS sound(9).
                // The HyperJump low tone is generated continuously in the audio
                // callback, not as blocking sound(9) calls. In particular the
                // phase-4 deceleration must never accumulate a pulse backlog.
                if(space.hyperPhase==0 && space.frontView)
                    spaceFrontPulsePending+=pulses;
            }
            pushMessages(sm);
            // A star collision, alien shot or drained shield can terminate SPACE
            // from updateFrame().  Do not require an extra keyboard command before
            // the resurrection path runs.
            resurrectIfNeeded();
        }else{
            spaceTopTimingActive=false;
            spaceFrontTimingActive=false;
            spaceFrontRetraceBudget=0;
        }

        // OUT.EXE resurrection presentation: skull first, then three synchronous
        // sound(2) calls, then the death line.  The death line remains visible
        // for delay(0x50) BIOS ticks before the resurrection line.  Hits then
        // starts immediately at 1 and rises to 99 at one BIOS tick per step;
        // Food follows with the same cadence.
        if(resurrectionPhase!=ResurrectionPhase::Inactive){
            const std::uint32_t now=SDL_GetTicks();
            if(resurrectionPhase==ResurrectionPhase::DeadHold &&
               resurrectionDeadShownMs!=0 &&
               static_cast<std::uint32_t>(now-resurrectionDeadShownMs)>=RESURRECTION_DEAD_HOLD_MS){
                log.push("부활을 시도합니다!");
                p.hits=1;
                resurrectionPhase=ResurrectionPhase::HitsRising;
                resurrectionStepMs=now;
            }else if(resurrectionPhase==ResurrectionPhase::HitsRising){
                while(p.hits<99 && static_cast<std::uint32_t>(now-resurrectionStepMs)>=RESURRECTION_STAT_STEP_MS){
                    ++p.hits;
                    resurrectionStepMs+=RESURRECTION_STAT_STEP_MS;
                }
                if(p.hits>=99){
                    p.hits=99;
                    resurrectionPhase=ResurrectionPhase::HitsAt99;
                }
            }else if(resurrectionPhase==ResurrectionPhase::HitsAt99){
                // OUT.EXE updates the status to 99 and waits one timer tick before
                // monster cleanup and the first Food increment.
                if(static_cast<std::uint32_t>(now-resurrectionStepMs)>=RESURRECTION_STAT_STEP_MS){
                    resurrection.afterHitsAnimated(p);
                    p.food=1;
                    resurrectionPhase=ResurrectionPhase::FoodRising;
                    resurrectionStepMs=now;
                }
            }else if(resurrectionPhase==ResurrectionPhase::FoodRising){
                while(p.food<99 && static_cast<std::uint32_t>(now-resurrectionStepMs)>=RESURRECTION_STAT_STEP_MS){
                    ++p.food;
                    resurrectionStepMs+=RESURRECTION_STAT_STEP_MS;
                }
                if(p.food>=99){
                    p.food=99;
                    resurrectionPhase=ResurrectionPhase::FoodAt99;
                }
            }else if(resurrectionPhase==ResurrectionPhase::FoodAt99){
                // Preserve the final Food=99 skull frame for the same one-tick
                // cadence before OUT.EXE resumes the relocated overworld.
                if(static_cast<std::uint32_t>(now-resurrectionStepMs)>=RESURRECTION_STAT_STEP_MS){
                    // OUT.EXE 0xC7A5: after the final Food=99 cadence,
                    // sound(5) completes before 0x8E20 relocates the player
                    // and normal overworld play resumes.
                    if(noiseEnabled) sfx.play(SfxEvent::ResurrectionComplete);
                    resurrection.finishAnimated(p);
                    resurrectionPhase=ResurrectionPhase::Inactive;
                    resurrectionStepMs=0;
                    resurrectionDeadShownMs=0;
                    overworldAnimEpochMs=SDL_GetTicks();
                    overworldAnimProcessedStep=0;
                    worldObjectAnimBit.fill(0);
                }
            }
        }

        // Timer starts on the first render-loop turn with the defeat popup active.
        if(mondain.active && mondain.defeated){
            const std::uint32_t now=SDL_GetTicks();
            if(mondainDefeatPopupStartedMs==0) mondainDefeatPopupStartedMs=now;
            else if(static_cast<std::uint32_t>(now-mondainDefeatPopupStartedMs)>=MONDAIN_DEFEAT_POPUP_MS){
                mondainSystem.acknowledgeDefeat(mondain);
                mondainDefeatPopupStartedMs=0;
                resurrectIfNeeded();
            }
        }else mondainDefeatPopupStartedMs=0;
        const bool mondainIntroBlocking=mondain.active && mondain.introActive;
        const std::uint32_t mondainNowForVictory=SDL_GetTicks();
        if(mondain.active && mondain.victorious &&
           (mondain.endingDissolveActive || mondain.endingActive))
            mondainSystem.updateEndingPresentation(mondain,mondainNowForVictory);
        const bool mondainEndingFullScreen=
            mondain.active && mondain.victorious && mondain.endingActive;

        // MONDAIN.EXE input reader 0x05DB calls animation routine 0x1477 on
        // every keyboard-poll cycle; with no key pending it waits 3 BIOS ticks.
        // Advance final-arena visual state on that source cadence so the evil
        // gem, transient flame/magic tiles, and Mondain animate while waiting.
        const bool mondainInputWait = mondain.active && !mondain.introActive &&
                                      !mondain.defeated && !mondain.victorious &&
                                      !pendingMondainAttackResolve && !pendingMondainMoveFinish;
        if(mondainInputWait){
            const std::uint32_t mondainNow=SDL_GetTicks();
            if(mondainAnimEpochMs==0){
                mondainAnimEpochMs=mondainNow;
                mondainAnimProcessedStep=0;
            }
            const std::uint32_t mondainStep=(mondainNow-mondainAnimEpochMs)/OVERWORLD_ANIM_MS;
            while(mondainAnimProcessedStep<mondainStep){
                ++mondainAnimProcessedStep;
                mondainSystem.animateInputWaitTick(mondain,simulation);
            }
        }else{
            mondainAnimEpochMs=SDL_GetTicks();
            mondainAnimProcessedStep=0;
        }

        SDL_SetRenderDrawColor(r,0,0,0,255);SDL_RenderClear(r);

        if(resurrectionPhase!=ResurrectionPhase::Inactive){
            drawResurrectionSkull(r);
        } else if(space.active){
            spaceSystem.draw(r,font,space,p,WHITE);
        } else if(mondain.active && !mondain.introActive){
            mondainSystem.draw(r,font,mondain);
        } else if(dungeon.active){
            dungeonSystem.drawWireframe(r,
                dungeonLethalPrePresent ? dungeonLethalVisualSnapshot : dungeon,WHITE);
            // M9 has exact monster simulation/combat. The geometric monster silhouettes
        } else if(local.active){
            // TCD local maps are exactly 38x18 8x8 tiles = 304x144 DOS pixels.
            // At the 2x presentation size that is 608x288, filling the same viewport.
            for(int x=0;x<OriginalData::LOCAL_W;++x){
                for(int y=0;y<OriginalData::LOCAL_H;++y){
                    int tile=simulation.displayTile(local,x,y);
                    // Castle zone names are not painted into a background image:
                    // TCD stores them as ordinary EGATOWN A..Z character tiles.
                    // Replace only those source cells with the castle's solid-white
                    // wall tile, preserving collision/raw-map semantics unchanged.
                    if(!local.inTown && castleZoneLabelCell(local.mapIndex,x,y))
                        tile=0;
                    // M166: erase the original English sign cells back to the
                    // surrounding wall.  Only the Korean glyph cells themselves
                    // are repainted as inverse-white blocks by drawTownWallLabels().
                    if(local.inTown && townWallLabelCell(local.mapIndex,x,y)){
                        int sampleX=x, sampleY=y;
                        bool sampled=false;
                        forEachTownWallLabel([&](const TownWallLabel& label){
                            if(sampled || label.mapIndex!=local.mapIndex ||
                               x<label.x0 || x>label.x1 || y<label.y0 || y>label.y1) return;
                            if(label.y0==label.y1){
                                const int lx=label.x0-1, rx=label.x1+1;
                                if(lx>=0 && !townWallLabelCell(local.mapIndex,lx,y)){
                                    sampleX=lx; sampleY=y; sampled=true;
                                }else if(rx<OriginalData::LOCAL_W && !townWallLabelCell(local.mapIndex,rx,y)){
                                    sampleX=rx; sampleY=y; sampled=true;
                                }
                            }else{
                                const int lx=x-1, rx=x+1;
                                if(lx>=0 && !townWallLabelCell(local.mapIndex,lx,y)){
                                    sampleX=lx; sampleY=y; sampled=true;
                                }else if(rx<OriginalData::LOCAL_W && !townWallLabelCell(local.mapIndex,rx,y)){
                                    sampleX=rx; sampleY=y; sampled=true;
                                }
                            }
                        });
                        if(sampled) tile=simulation.displayTile(local,sampleX,sampleY);
                    }
                    SDL_Texture* tex=original.localTexture(tile);
                    SDL_Rect dst{VIEW_X+x*LOCAL_TILE,VIEW_Y+y*LOCAL_TILE,LOCAL_TILE,LOCAL_TILE};
                    SDL_RenderCopy(r,tex,nullptr,&dst);
                }
            }
            // Local-map Korean wall labels are composited after drawFrames().
            // This prevents the gameplay inner white rail from cutting through
            // Hangul glyphs that touch the viewport edge.

            // OUT.EXE maintains local actors as a separate 15-record layer.
            // Render the exact tile/x/y copied from the original actor template table.
            for(const auto& a:local.actors){
                if(!a.active()) continue;
                SDL_Rect adst{VIEW_X+static_cast<int>(a.x)*LOCAL_TILE,
                              VIEW_Y+static_cast<int>(a.y)*LOCAL_TILE,LOCAL_TILE,LOCAL_TILE};
                SDL_RenderCopy(r,original.localTexture(static_cast<int>(a.tile)),nullptr,&adst);
            }
            // OUT.EXE local renderer draws player as EGATOWN tile 0x12.
            SDL_Rect pdst{VIEW_X+local.x*LOCAL_TILE,VIEW_Y+local.y*LOCAL_TILE,LOCAL_TILE,LOCAL_TILE};
            SDL_RenderCopy(r,original.localTexture(0x12),nullptr,&pdst);
        } else {
            // OUT.EXE uses c2ec/c31c for every 19x9 viewport coordinate, so the
            // 168x156 overworld wraps toroidally instead of clamping at an edge.
            const std::uint32_t now=SDL_GetTicks();

            // B157 belongs to OUT.EXE's ordinary overworld command-wait loop.
            // Modal routines (Ztats, shop, Ready, king input, attack-direction
            // input, spell direction, save delay) have their own blocking
            // input paths and must not silently consume monster-animation RNG.
            const bool normalOverworldWaitBase=
                !showZ && !shop.active() && !king.active() && !commands.active() &&
                pendingAttack==0 && !pendingAttackResolution && !worldProjectileActive && !worldPostHitSoundPending &&
                !enemyAttackVisualActive &&
                resurrectionPhase==ResurrectionPhase::Inactive &&
                !mondainIntroBlocking &&
                !shuttleLaunchCountdownActive &&
                !pendingQuitSave;
            const bool normalOverworldWait=mondainIntroBlocking || normalOverworldWaitBase;

            if(!mondainIntroBlocking && (overworldAnimEpochMs==0 || !normalOverworldWait)){
                overworldAnimEpochMs=now;
                overworldAnimProcessedStep=0;
                worldObjectAnimBit.fill(0);
            }
            const std::uint32_t animationNow=mondainIntroBlocking
                ? overworldAnimEpochMs+overworldAnimProcessedStep*OVERWORLD_ANIM_MS
                : now;
            const std::uint32_t animStep=normalOverworldWait
                ? (animationNow-overworldAnimEpochMs)/OVERWORLD_ANIM_MS
                : 0u;

            auto screenDeltaX=[](int objectX,int playerX){
                int q=objectX-playerX; if(q>83)q-=168; if(q<-84)q+=168; return q;
            };
            auto screenDeltaY=[](int objectY,int playerY){
                int q=objectY-playerY; if(q>77)q-=156; if(q<-78)q+=156; return q;
            };

            // OUT.EXE B157 loops the 19x9 viewport in x-major/y-minor order.
            // For every *visible* normal monster base tile it calls the global
            // DOS randomRange(1,100), then uses remainder %2 as the A/B frame.
            // Process skipped host frames quantum-by-quantum so SDL frame rate
            // cannot alter either animation or RNG consumption.
            while(overworldAnimProcessedStep<animStep){
                ++overworldAnimProcessedStep;
                for(int xx=0;xx<WORLD_COLS;++xx){
                    for(int yy=0;yy<WORLD_ROWS;++yy){
                        const int wx=wrapX(static_cast<int>(p.x)+(xx-WORLD_COLS/2));
                        const int wy=wrapY(static_cast<int>(p.y)+(yy-WORLD_ROWS/2));

                        int objectIndex=-1;
                        for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i){
                            const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
                            if(o.empty()) continue;
                            if(static_cast<int>(o.x())==wx && static_cast<int>(o.y())==wy){
                                objectIndex=i;
                                break;
                            }
                        }
                        if(objectIndex<0) continue;

                        const int baseTile=static_cast<int>(
                            p.worldObjects[static_cast<std::size_t>(objectIndex)].tile());
                        if(worldTileRandomPairBase(baseTile)){
                            worldObjectAnimBit[static_cast<std::size_t>(objectIndex)]=
                                static_cast<std::uint8_t>(simulation.randomRange(1,100)%2);
                        }
                    }
                }
            }

            for(int yy=0;yy<WORLD_ROWS;++yy)for(int xx=0;xx<WORLD_COLS;++xx){
                const int wx=wrapX(static_cast<int>(p.x)+(xx-WORLD_COLS/2));
                const int wy=wrapY(static_cast<int>(p.y)+(yy-WORLD_ROWS/2));
                SDL_Texture* tex=original.animatedTerrainTexture(original.terrainAt(wx,wy),animStep);
                SDL_Rect dst{VIEW_X+xx*WORLD_TILE,VIEW_Y+yy*WORLD_TILE,WORLD_TILE,WORLD_TILE};
                SDL_RenderCopy(r,tex,nullptr,&dst);
            }

            for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i){
                const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
                if(o.empty()) continue;
                const int sx=screenDeltaX(static_cast<int>(o.x()),static_cast<int>(p.x));
                const int sy=screenDeltaY(static_cast<int>(o.y()),static_cast<int>(p.y));
                if(sx<-WORLD_COLS/2||sx>WORLD_COLS/2||sy<-WORLD_ROWS/2||sy>WORLD_ROWS/2) continue;

                const int baseTile=static_cast<int>(o.tile());
                const int displayTile=worldAnimatedPhysicalTile(
                    baseTile,animStep,
                    worldObjectAnimBit[static_cast<std::size_t>(i)]);
                SDL_Texture* ot=original.physicalTexture(displayTile); if(!ot) continue;
                SDL_Rect od{VIEW_X+(WORLD_COLS/2+sx)*WORLD_TILE,VIEW_Y+(WORLD_ROWS/2+sy)*WORLD_TILE,WORLD_TILE,WORLD_TILE};
                SDL_RenderCopy(r,ot,nullptr,&od);
            }
            if(worldProjectileActive && worldProjectileIndex<worldProjectilePath.size()){
                const auto [sx,sy]=worldProjectilePath[worldProjectileIndex];
                SDL_Texture* effect=original.physicalTexture(50); // OUT.EXE caller 0x1257: BX=0x32
                if(effect){
                    SDL_Rect ed{VIEW_X+(WORLD_COLS/2+sx)*WORLD_TILE,
                                VIEW_Y+(WORLD_ROWS/2+sy)*WORLD_TILE,
                                WORLD_TILE,WORLD_TILE};
                    SDL_RenderCopy(r,effect,nullptr,&ed);
                }
            }
            SDL_Rect pdst{VIEW_X+(WORLD_COLS/2)*WORLD_TILE,VIEW_Y+(WORLD_ROWS/2)*WORLD_TILE,WORLD_TILE,WORLD_TILE};
            int playerTile=static_cast<int>(p.worldObjects[0].tile());
            if(playerTile<=0||playerTile>=52) playerTile=10;
            // A boarded Frigate is stored as physical base tile 14 and is also
            // seen by B157 at the center viewport cell, so it alternates 14/15.
            // Facing is applied only after this physical-frame selection, so both
            // 14 and 15 animate in either horizontal direction.
            playerTile=worldAnimatedPhysicalTile(playerTile,animStep,0);
            if(vehicleSpriteNeedsHorizontalFlip(static_cast<int>(p.transport),vehicleFacingLeft)){
                SDL_RenderCopyEx(r,original.physicalTexture(playerTile),nullptr,&pdst,
                                 0.0,nullptr,SDL_FLIP_HORIZONTAL);
            }else{
                SDL_RenderCopy(r,original.physicalTexture(playerTile),nullptr,&pdst);
            }

            if(enemyAttackVisualActive){
                int ex=static_cast<int>(p.x);
                int ey=static_cast<int>(p.y);

                if(!enemyAttackHitFlash &&
                   enemyAttackVisualIndex<enemyAttackVisual.path.size()){
                    ex=enemyAttackVisual.path[enemyAttackVisualIndex].first;
                    ey=enemyAttackVisual.path[enemyAttackVisualIndex].second;
                }

                const int esx=screenDeltaX(ex,static_cast<int>(p.x));
                const int esy=screenDeltaY(ey,static_cast<int>(p.y));
                if(esx>=-WORLD_COLS/2 && esx<=WORLD_COLS/2 &&
                   esy>=-WORLD_ROWS/2 && esy<=WORLD_ROWS/2){
                    SDL_Texture* effect=original.physicalTexture(50);
                    if(effect){
                        SDL_Rect ed{VIEW_X+(WORLD_COLS/2+esx)*WORLD_TILE,
                                    VIEW_Y+(WORLD_ROWS/2+esy)*WORLD_TILE,
                                    WORLD_TILE,WORLD_TILE};
                        SDL_RenderCopy(r,effect,nullptr,&ed);
                    }
                }
            }
        }

        // M258: all four MONDAIN.EXE Time Machine pages inherit the already-present OUT.EXE framebuffer.
        // While its first page is blocking, retain the frozen overworld view and
        // paint only the source popup over it instead of replacing the viewport.
        if(mondainIntroBlocking) mondainSystem.draw(r,font,mondain);

        // M258: each Time Machine narration page is a modal inside the normal Ultima frame,
        // not a full-screen replacement.  Keep frame, command pane and status pane
        // visible exactly as in the DOS presentation; suppress only competing modal
        // overlays and Ready/input widgets while the page is waiting for Space/Return.
        if(!mondainEndingFullScreen) drawFrames(r);
        // HF35 regression guard: the inner white gameplay frame must exist
        // before Korean castle/town wall labels are painted. Otherwise the rail
        // slices through Hangul glyphs along the viewport edge.
        if(!mondainEndingFullScreen && local.active){
            if(!local.inTown) drawCastleZoneLabels(r,font,local.mapIndex);
            else drawTownWallLabels(r,font,local.mapIndex);
        }
        if(dungeon.active){
            drawDungeonFramedLabel(r,font,std::to_string(dungeon.level)+"층",0,true);
            const char* facing="";
            switch(dungeon.orientation){
                case 1:facing="서쪽";break;
                case 2:facing="동쪽";break;
                case 3:facing="북쪽";break;
                case 4:facing="남쪽";break;
            }
            drawDungeonFramedLabel(r,font,facing,302,false);
        }
        if(!mondainIntroBlocking){
            if(!space.active) drawKingServicePopup(r,font,king);
            if(!space.active && princessRescuePresentation.active())
                drawPrincessRescuePopup(r,font,princessRescuePopupLines);
            if(!space.active && landmarkPresentation.active())
                drawLandmarkPopup(r,font,landmarkPopupLines,
                    landmarkSfxPending==5 && !landmarkQuestSfxPending &&
                    !landmarkBlueRestorePending);
            if(!space.active) drawTimeMachineUnreadyPopup(r,font,timeMachineUnreadyPopup);
            if(!space.active && !timeMachineUnreadyPopup && !commandHelpPopup && !questPopup)
                drawReadyPopup(r,font,commands,p);
        }
        const std::string modalInput=
            mondainIntroBlocking ? std::string("스페이스바를 눌러 계속하세요: ") :
            (questPopup ? std::string("스페이스바를 눌러 계속하세요:  ") :
            (commandHelpPopup ? std::string("스페이스바를 눌러 계속하세요:  ") :
            (showZ ? std::string("스페이스바를 눌러 계속하세요: ") :
            (king.active()?king.inputLine():std::string{}))));
        const bool spaceHyperBlocking=space.active && space.hyperPhase!=0;
        if(!mondainEndingFullScreen) drawLog(r,font,original,log,modalInput,
                resurrectionPhase==ResurrectionPhase::Inactive &&
                !(mondain.active && mondain.defeated) &&
                !shuttleLaunchCountdownActive &&
                !(space.active && space.sectorScan) &&
                !spaceHyperBlocking &&
                !(mondain.active && mondain.victorious) &&
                !timeMachineUnreadyPopup,
                spaceHyperBlocking);
        if(!mondainIntroBlocking && !(mondain.active && mondain.victorious) &&
           !timeMachineUnreadyPopup && !commandHelpPopup && !questPopup){
            if(commands.active()) drawReadyCommandInput(r,font,original,commands);
            if(!pendingAttackLine.empty())
                drawAttackCommandInput(r,font,original,pendingAttackLine,pendingAttack!=0);
            if(!pendingMondainAttackLine.empty())
                drawAttackCommandInput(r,font,original,pendingMondainAttackLine,pendingMondainAttack);
            if(!pendingMondainCastLine.empty())
                drawAttackCommandInput(r,font,original,pendingMondainCastLine,pendingMondainCastSpell!=0);
            if(!pendingWorldCastLine.empty())
                drawAttackCommandInput(r,font,original,pendingWorldCastLine,pendingWorldCastSpell!=0);
            if(shop.active() && !shop.inputLine().empty())
                drawAttackCommandInput(r,font,original,shop.inputLine(),true);
            else if(shop.active() && shop.stage()==TradeStage::Message && !pendingShopResultLine.empty())
                drawAttackCommandInput(r,font,original,pendingShopResultLine,false);
        }
        if(!mondainEndingFullScreen){
            if(!space.active) drawStatus(r,font,p);
        }
        if(!mondainIntroBlocking){
            if(showZ) drawZtatsInventory(r,font,p,ztatsPage);
            if(questPopup) drawQuestStatusPopup(r,font,questJournal);
            if(commandHelpPopup) drawCommandHelpPopup(r,font);
            if(shop.active()) drawShopPopupM149(r,font,shop,p);
        }
        // Apple-II-inspired victory transition.  Work in the 320x200
        // source-pixel grid so every dissolved point becomes one 2x2 block in
        // the 640x400 host raster.  A stable integer hash gives a random-looking
        // order without consuming gameplay RNG.
        if(mondain.active && mondain.victorious && mondain.endingDissolveActive){
            const std::uint32_t elapsed=SDL_GetTicks()-mondain.endingDissolveStartedMs;
            const std::uint32_t dissolveElapsed=
                elapsed>1000u ? elapsed-1000u : 0u;
            const std::uint32_t progress=std::min<std::uint32_t>(1000u,dissolveElapsed);
            SDL_SetRenderDrawColor(r,0,0,0,255);
            for(int sy=0;sy<200;++sy){
                for(int sx=0;sx<320;++sx){
                    std::uint32_t h=static_cast<std::uint32_t>(sx+sy*320);
                    h^=h>>16; h*=0x7feb352du; h^=h>>15; h*=0x846ca68bu; h^=h>>16;
                    if((h%1000u)>=progress) continue;
                    SDL_Rect q{sx*2,sy*2,2,2};
                    SDL_RenderFillRect(r,&q);
                }
            }
        }

        const bool captureStageReady=
            !testTownArmourBuy || (shop.active() && shop.stage()==TradeStage::Item);
        if(!captureBmp.empty() && captureStageReady){
            std::string capErr;
            if(!saveRendererBmp(r,captureBmp,W,H,capErr)){std::cerr<<capErr<<"\n";return 6;}
            captureBmp.clear();
            if(captureOnce) run=false;
        }

        // M264: remember whether this *rendered* frame actually carried the
        // negative.  A psionic latch can be raised by finishCommand() after a
        // prior present; such a late latch must survive until the next frame.
        const bool mondainPsionicFramePresented=mondain.active && mondain.psionicInvertPending;
        // M273: MONDAIN.EXE 0x1EA8 blits corpse slot 4 before constructing the
        // victory popup.  Record which victory stage this rendered frame contains
        // so post-present sound cannot run against the preceding bat/human frame.
        const bool mondainVictoryCorpseFramePresented =
            mondain.active && mondain.victorious && mondain.victoryPopup && mondain.victoryDrawPhase==0;
        const bool mondainVictoryPopupFramePresented =
            mondain.active && mondain.victorious && mondain.victoryPopup && mondain.victoryDrawPhase>=1;
        const bool landmarkBlueRestoreFramePresented =
            landmarkPresentation.active() && landmarkBlueRestorePending;
        if(mondainPsionicFramePresented || mondainCollisionFlashPending)
            invertCurrentGameFrame();

        // M231: OUT.EXE draws first, then enters the synchronous PC-speaker
        // routine.  Do not start those sounds before presentation; present the
        // newly blitted frame first and execute sound() in the post-present
        // section below while that frame remains visible.
        presentFrame();
        if(mondainCollisionFlashPending){
            // The inverted frame must actually be presented while source sound(2)
            // runs; the normal frame returns on the next render iteration.
            if(noiseEnabled) sfx.play(2);
            mondainCollisionFlashPending=false;
        }
        // OUT.EXE 0x108B..0x10B4 shows hit/damage before the lethal tail
        // at 0x112A.  Never hide the target before the damage frame is presented.
        if(dungeonLethalPrePresent){
            dungeonLethalPrePresent=false;
            pushMessages(dungeonLethalDeferred);
            dungeonLethalDeferred.clear();
            dungeonLethalResultNeedsPresent=true;
        }else if(dungeonLethalResultNeedsPresent){
            dungeonLethalResultNeedsPresent=false;
            if(!dungeonPlayerSfxFinishPending) finishCommand();
        }
        // The complete page and prompt must reach SDL_RenderPresent once
        // before MONDAIN.EXE-equivalent Space/Return polling can consume input.
        if(mondainIntroBlocking && mondain.introDrawPhase==0)
            mondain.introDrawPhase=1;
        // Final death/corpse frame hands off immediately to the victory
        // popup on the next host present; there is no added death->popup delay.
        const std::uint32_t victoryPresentNow=SDL_GetTicks();
        if(mondainVictoryCorpseFramePresented && mondain.victoryDrawPhase==0)
            mondain.victoryDrawPhase=1;
        if(mondainVictoryPopupFramePresented && !mondain.victoryPopupPresented){
            mondain.victoryPopupPresented=true;
            mondain.victoryPopupPresentedMs=victoryPresentNow;
        }

        if(resurrectionPhase==ResurrectionPhase::SkullIntro){
            // OUT.EXE 0xC6BD..0xC6D6: the skull is already visible when three
            // synchronous sound(2) calls execute.  The death text is printed
            // only after the third call returns.
            if(noiseEnabled){
                sfx.play(SfxEvent::CombatHit);
                sfx.play(SfxEvent::CombatHit);
                sfx.play(SfxEvent::CombatHit);
            }
            log.push(p.name+"님, 당신은 사망하였습니다.");
            resurrectionPhase=ResurrectionPhase::DeadHold;
            resurrectionDeadShownMs=0;
        }else if(resurrectionPhase==ResurrectionPhase::DeadHold && resurrectionDeadShownMs==0){
            // Start delay(0x50) only after the death-text frame is presented.
            resurrectionDeadShownMs=SDL_GetTicks();
        }

        princessRescuePresentation.afterPresent(SDL_GetTicks());
        landmarkPresentation.afterPresent(SDL_GetTicks());
        if(showZ) ztatsPagePresented=true;
        if(firstHelpPending && firstHelpWorldFrameMs==0 && !frontend.active())
            firstHelpWorldFrameMs=SDL_GetTicks();
        if(commandHelpPopup) commandHelpPresented=true;
        if(questPopup) questPopupPresented=true;
        if(commands.selectorPopupActive()) selectorPopupPresented=true;
        if(shop.active() && shop.stage()==TradeStage::Message)
            shopMessagePresented=true;
        if(timeMachineUnreadyPopup) timeMachineUnreadyPresented=true;
        if(king.active() && king.stage()==KingStage::ServicePopup)
            kingServicePopupPresented=true;

        // OUT.EXE landmark ordering: optional quest completion sound(5), then
        // final reward sound(5/6).  A successful reward is shown with a white
        // outline during sound(5), then restored to blue before 0x2341 polls.
        if(landmarkPresentation.active() && landmarkQuestSfxPending){
            landmarkQuestSfxPending=false;
            sfx.play(5);
        }else if(landmarkPresentation.active() && landmarkSfxPending>=0){
            const int id=landmarkSfxPending;
            landmarkSfxPending=-1;
            sfx.play(id);
            if(id==5) landmarkBlueRestorePending=true;
        }
        if(landmarkBlueRestoreFramePresented)
            landmarkBlueRestorePending=false;
        if(landmarkPresentation.active() && landmarkDismissQueued &&
           !landmarkQuestSfxPending && landmarkSfxPending<0 &&
           !landmarkBlueRestorePending){
            landmarkDismissQueued=false;
            landmarkPresentation.clear();
            landmarkPopupLines.clear();
            finishCommand();
        }

        // M129: the dialogue is already in the log before rendering.  Once
        // that frame has reached SDL_RenderPresent, keep it visible for 0.1 s
        // and only then reopen command input.
        if(localActorDialoguePresentation.awaitingDialoguePresent()){
            SDL_Delay(LOCAL_ACTOR_DIALOGUE_POST_DELAY_MS);
            localActorDialogueRows.clear();
            localActorDialoguePresentation.finish();
            finishCommandTail();
        }

        const std::uint32_t attackPresentNow=SDL_GetTicks();

        // MONDAIN.EXE 0x0718..0x0796: successful avatar movement is already
        // visible when sound(4) runs.  Only after that synchronous source call
        // returns does the final-arena command complete and Mondain's turn run.
        if(pendingMondainMoveFinish){
            pendingMondainMoveFinish=false;
            if(noiseEnabled) sfx.play(SfxEvent::FinalArenaPlayerMove);
            finishCommand();
        }

        // MONDAIN.EXE 0x0A5C -> 0x09C0 -> 0x0831: after the chosen
        // direction has been printed on the live Attack row, attack core 0x0831
        // begins with sound(7), before hit/ray resolution.
        if(pendingMondainAttackResolve){
            pendingMondainAttackResolve=false;
            if(noiseEnabled) sfx.play(SfxEvent::FinalArenaPlayerAttack);
            const int dir=pendingMondainAttackDir;
            pendingMondainAttackDir=0;
            log.pushCommand(pendingMondainAttackLine);
            pendingMondainAttackLine.clear();
            std::vector<std::string> mm;
            if(dir) mondainSystem.attack(mondain,p,simulation,dir,mm);
            pushMessages(mm);
            if(mondain.playerAttackHitSfxPending){
                mondain.playerAttackHitSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::CombatHit);
            }
            finishCommand();
        }

        // Local move: 0x30E9..0x3117 has already been presented.  The next
        // original instruction sequence is sound(4), and only after it returns
        // does the command return to the actor-update path.
        if(localPlayerMoveSoundPending){
            localPlayerMoveSoundPending=false;
            if(noiseEnabled) sfx.play(SfxEvent::LocalPlayerMove);
            finishCommand();
        }

        // M271 final-arena response sounds. Successful evil-gem destruction
        // executes MONDAIN.EXE 0x10EB: sound(2) five times. A lethal hit keeps
        // the ordinary damage sound(2); after the victory popup has reached
        // the display, the original victory routine 0x1EA8/0x1EF5 executes
        // sound(5) exactly three times. Psionic audio remains presentation-bound.
        if(mondain.active){
            if(mondain.playerSpellSfxPending){
                mondain.playerSpellSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::PositiveEffect); // MONDAIN player spell sound(5)
            }
            if(mondain.playerSpellHitSfxPending){
                mondain.playerSpellHitSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::CombatHit); // Magic Missile terminal hit sound(2)
            }
            if(mondain.playerSpellFailSfxPending){
                mondain.playerSpellFailSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::NoEffect); // failed/no-effect spell sound(6)
            }
            if(mondain.gemDestroySfxPending){
                mondain.gemDestroySfxPending=false;
                if(noiseEnabled){
                    for(int i=0;i<original.mondainGemDestroySoundCount();++i)
                        sfx.play(original.mondainGemDestroySoundId());
                }
            }
            if(mondain.victoryPopupSfxPending &&
               mondainVictoryPopupFramePresented &&
               mondain.victoryPopupPresented &&
               static_cast<std::uint32_t>(SDL_GetTicks()-mondain.victoryPopupPresentedMs)>=1000u){
                mondain.victoryPopupSfxPending=false;
                if(noiseEnabled){
                    sfx.play(SfxEvent::PositiveEffect);
                    sfx.play(SfxEvent::PositiveEffect);
                    sfx.play(SfxEvent::PositiveEffect);
                }
                // After the third BEEP returns, acknowledgeVictory starts the
                // existing one-second intact-popup hold before dissolve-out.
                mondainSystem.acknowledgeVictory(mondain,SDL_GetTicks());
            }
            const bool psionicNegativeVisible=mondain.psionicInvertPending && mondainPsionicFramePresented;
            const bool deferPsionicSpell=mondain.psionicInvertPending && !mondainPsionicFramePresented;
            if(mondain.enemyAttackSfxPending){
                mondain.enemyAttackSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::FinalArenaEnemyAttack); // melee sound(3)
            }
            if(mondain.enemySpellSfxPending && !deferPsionicSpell){
                mondain.enemySpellSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::PositiveEffect); // MONDAIN spell sound(5)
            }
            if(mondain.enemyHitSfxPending && !mondain.psionicInvertPending){
                mondain.enemyHitSfxPending=false;
                if(noiseEnabled) sfx.play(SfxEvent::CombatHit);
            }
            if(psionicNegativeVisible){
                // Original 0x180E..0x181A: negative palette -> sound(5) -> restore.
                // The hit/miss roll and damage helper occur only after restore.
                mondain.psionicInvertPending=false;
            }else if(mondain.psionicHitAfterRestorePending && !mondain.psionicInvertPending){
                mondain.psionicHitAfterRestorePending=false;
                if(noiseEnabled) sfx.play(SfxEvent::CombatHit);
            }
        }

        // OUT.EXE 0x2B4E local guard retaliation: every adjacent guard
        // attack starts with sound(7); only a damaging hit appends sound(2).
        // Drain in attack order after the resulting local frame is visible.
        while(!localGuardSfxQueue.empty()){
            const int id=localGuardSfxQueue.front();
            localGuardSfxQueue.pop_front();
            if(noiseEnabled) sfx.play(id);
        }

        // Dungeon monster response sounds are generated by finishCommand() and
        // therefore become audible only after their result rows have reached
        // the next presented frame. Each attack uses sound(3); normal damage
        // hits append sound(2), exactly matching OUT.EXE 0x8A06/0x8BBE.
        if(!dungeonEnemySfxQueue.empty()){
            while(!dungeonEnemySfxQueue.empty()){
                const int id=dungeonEnemySfxQueue.front();
                dungeonEnemySfxQueue.pop_front();
                if(noiseEnabled) sfx.play(id);
            }
        }

        // Dungeon player attack: preserve the original two independent sound
        // calls. A valid attack always has sound(7); only a hit adds sound(2).
        if(dungeonPlayerSfxFinishPending){
            while(!dungeonPlayerSfxQueue.empty()){
                const int id=dungeonPlayerSfxQueue.front();
                dungeonPlayerSfxQueue.pop_front();
                if(noiseEnabled) sfx.play(id);
            }
            dungeonPlayerSfxFinishPending=false;
            if(!dungeonLethalResultNeedsPresent) finishCommand();
        }

        // Result/BEEP effects are likewise source-synchronous after their text
        // frame is visible.  play() itself is the entire hold.
        if(commandSfxHoldPending){
            const int id=commandSfxHoldId;
            const int continuation=commandSfxHoldContinuation;
            commandSfxHoldPending=false;
            commandSfxHoldId=-1;
            commandSfxHoldContinuation=0;
            if(noiseEnabled) sfx.play(id);
            if(continuation==1) finishCommand();
            else if(continuation==2) pendingAttackResolution=true;
        }

        // Apple II Front View STAR/FGT3 speaker remains active while HyperJump
        // accelerates/decelerates (phases 1/4).  The original Apple II BASIC
        // keeps calling USR(5) with changing DE values there; entering HyperJump
        // must not silence the high layer before the lightspeed section.
        // Phases 2/3 are handled separately below because their DE=1 tight loop
        // has a different, broken/noisy texture.
        const bool appleFrontSound =
            noiseEnabled && space.active && space.frontView &&
            (space.hyperPhase>=0 && space.hyperPhase<=4) &&
            (space.hyperPhase!=0 || (
                !spaceFireSoundPending && !space.playerHitSfxPending &&
                !space.alienFireSfxPending && !space.alienHitSfxPending &&
                space.alienShotFrames==0));
        // Apple II SPA MOVE uses DE=1 during both lightspeed passes.
        // Keep the high layer at the pitch from before the jump; only the
        // independently synthesized low layer follows the speed ramp.
        // HF59: keep the same 1986 speaker trace engine through ramp;
        // only use original entry speed during the two tight-loop phases.
        const int starSpeed=(space.hyperPhase==2 || space.hyperPhase==3)
            ? std::clamp(space.hyperOriginalSpeed,1,8)
            : std::clamp(space.frontSpeed,1,8);
        sfx.setAppleFrontView(appleFrontSound,starSpeed,space.hyperPhase,
                              space.frontSpeed);

        if(space.crunchSfxPending){
            space.crunchSfxPending=false;
            if(noiseEnabled) sfx.play(SfxEvent::SpaceCrunch);
        }

        if(spaceFireSoundPending){
            if(noiseEnabled) sfx.play(SfxEvent::VehicleFire);
            spaceFireSoundPending=false;
            spaceFrontPulsePending=0;
            spaceHyperPulsePending=0;
        }
        if(space.playerHitSfxPending){
            space.playerHitSfxPending=false;
            if(noiseEnabled) sfx.play(SfxEvent::SpacePlayerHit);
        }

        if(space.alienFireSfxPending){
            space.alienFireSfxPending=false;
            if(noiseEnabled) sfx.play(SfxEvent::SpaceAlienFire);
            spaceFrontPulsePending=0;
            spaceHyperPulsePending=0;
        }
        if(space.alienHitSfxPending && space.alienShotFrames==0){
            space.alienHitSfxPending=false;
            if(noiseEnabled) sfx.play(SfxEvent::SpaceAlienHit);
        }

        if(spaceHyperPulsePending || spaceFrontPulsePending){
            // HF58 Apple II 1986 research trial: in tight-loop phases the
            // original SPA $C030 trace already drives the speaker. Do not
            // overlay the DOS-derived FlightPulse clips on top of it.
            const bool apple1986TightLoop=appleFrontSound &&
                (space.hyperPhase==2 || space.hyperPhase==3);
            const int pulses=appleFrontSound?0:
                (spaceHyperPulsePending+spaceFrontPulsePending);
            if(noiseEnabled){
                for(int i=0;i<pulses;++i) sfx.play(SfxEvent::SpaceFlightPulse);
            }
            spaceHyperPulsePending=0;
            spaceFrontPulsePending=0;
        }

        if(worldPostHitSoundPending && !worldPostHitSoundStarted){
            worldPostHitSoundStarted=true;
            worldPostHitSoundStartedMs=attackPresentNow;
            if(noiseEnabled) sfx.play(SfxEvent::CombatHit);
            worldPostHitSoundPending=false;
            worldPostHitSoundStarted=false;
            worldPostHitSoundStartedMs=0;
            finishCommand();
        }

        if(worldProjectileActive && !worldProjectileFramePresented){
            worldProjectileStepStartedMs=attackPresentNow;
            if(worldProjectileIndex==0 && worldProjectileFirstSoundId>=0){
                worldProjectileFirstSoundStartedMs=attackPresentNow;
                if(noiseEnabled) sfx.play(worldProjectileFirstSoundId);
            }
            worldProjectileFramePresented=true;
        }

        if(enemyAttackVisualActive && !enemyAttackVisualFramePresented){
            enemyAttackVisualStartedMs=attackPresentNow;
            if(noiseEnabled){
                if(enemyAttackHitFlash) sfx.play(SfxEvent::CombatHit);
                else if(enemyAttackVisualIndex==0) sfx.play(SfxEvent::EnemyRangedLaunch);
            }
            enemyAttackVisualFramePresented=true;
        }
    }

    SDL_SetRenderTarget(r,nullptr);
    SDL_DestroyTexture(mondainInvertScratch);
    SDL_DestroyTexture(frame);
    sfx.shutdown();SDL_DestroyRenderer(r);SDL_DestroyWindow(win);SDL_Quit();return 0;
}